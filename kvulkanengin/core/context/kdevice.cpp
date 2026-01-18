#include "kdevice.h"
#include <vector>
#include <algorithm>
#include <assert.h>

KDevice::~KDevice()
{
	Destroy();
}

bool KDevice::Create(const KPhysicalDevice& physicalDevice, const KDeviceCreateDesc& desc)
{
	m_queueFamiliesIdxs = physicalDevice.GetQueueFamilies();
	m_msaaSamples = physicalDevice.GetMaxUsableSampleCount();

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	QueueFamilysMap queueFamilies = _mergeQueueFamilyInfo(desc);
	for (auto& [_, info] : queueFamilies)
	{
		VkDeviceQueueCreateInfo qci{};
		qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		qci.queueFamilyIndex = info._familyIndex;
		qci.queueCount = info._requiredCount;
		qci.pQueuePriorities = info._priorities.data();

		queueCreateInfos.push_back(qci);
	}

	// 配置设备特性
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = desc._enabledFeatures._samplerAnisotropy; // 启用各向异性过滤
	deviceFeatures.sampleRateShading = desc._enabledFeatures._sampleRateShading; // 启用采样率着色

	// 创建逻辑设备
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()); // 队列创建信息数量
	createInfo.pQueueCreateInfos = queueCreateInfos.data(); // 队列创建信息
	createInfo.enabledExtensionCount = static_cast<uint32_t>(desc._enabledExtensions.size()); // 设备扩展数量
	createInfo.ppEnabledExtensionNames = desc._enabledExtensions.data(); // 设备扩展名称
	createInfo.pEnabledFeatures = &deviceFeatures; // 设备特性

	// 创建逻辑设备
	if (vkCreateDevice(physicalDevice.GetHandle(), &createInfo, nullptr, &m_logicDevice) != VK_SUCCESS)
		return false;

	// 获取逻辑设备队列
	vkGetDeviceQueue(m_logicDevice, m_queueFamiliesIdxs._graphicsFamily, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_logicDevice, m_queueFamiliesIdxs._presentFamily, 0, &m_presentQueue);

	return true;
}

void KDevice::Destroy()
{
	if (m_logicDevice != VK_NULL_HANDLE)
	{
		vkDestroyDevice(m_logicDevice, nullptr);
		m_logicDevice = VK_NULL_HANDLE;
	}
}

void  KDevice::SubmitGraphics(const VkSubmitInfo& submit, VkFence fence) const
{
	VkResult res = vkQueueSubmit(m_graphicsQueue, 1, &submit, fence);
	assert(res == VK_SUCCESS);
}

KDevice::QueueFamilysMap KDevice::_mergeQueueFamilyInfo(const KDeviceCreateDesc& desc) const
{
	QueueFamilysMap familyInfos;
	for (const auto& req : desc._queueRequests)
	{
		uint32_t familyIndex = UINT32_MAX;

		switch (req._type)
		{
		case QueueType::Graphics:
			familyIndex = m_queueFamiliesIdxs._graphicsFamily;
			break;
		case QueueType::Present:
			familyIndex = m_queueFamiliesIdxs._presentFamily;
			break;
		default:
			continue;
		}

		auto& info = familyInfos[familyIndex];
		info._familyIndex = familyIndex;
		info._requiredCount = std::max<const uint32_t>(info._requiredCount, req._count);
	}

	for (auto& [_, info] : familyInfos)
	{
		info._priorities.resize(info._requiredCount, 1.0f);
	}

	return familyInfos;
}
