#include "kphysicaldevice.h"

bool KPhysicalDevice::Pick(VkInstance instance, VkSurfaceKHR surface, const KDeviceCreateDesc& desc)
{
	_reset(); // 重置状态

	// 1. 获取物理设备数量
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	// 2. 获取所有物理设备
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	if (devices.empty())
		return false;

	// 3. 选择适合的物理设备
	for (const auto& device : devices)
	{
		if (_isDeviceSuitable(device, surface, desc))
		{
			m_physicalDevice = device;
			_findMaxUsableSampleCount();
			return true;
		}
	}

	return false;
}

bool KPhysicalDevice::QuerySwapChainSupport(VkSurfaceKHR surface, SwapChainSupportDetails& details) const
{
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, surface, &details._capabilities);

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface, &formatCount, nullptr);
	if (!formatCount)
		return false;

	details._formats.resize(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface, &formatCount, details._formats.data());

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, surface, &presentModeCount, nullptr);
	if (!presentModeCount)
		return false;

	details._presentModes.resize(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, surface, &presentModeCount, details._presentModes.data());

	return true;
}

bool KPhysicalDevice::_isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const KDeviceCreateDesc& desc)
{
	// 获取设备属性
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// 获取设备支持的 Vulkan 特性
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	// 独立显卡：VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU（性能最好）
	// 集成显卡：VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU（性能一般）
	// 虚拟化平台 / 集成显卡：VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU
	return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
		&& deviceFeatures.geometryShader // 支持几何着色器
		&& deviceFeatures.samplerAnisotropy // 支持各向异性过滤
		&& _findQueueFamilies(device, surface, desc) // 找到队列族
		&& _checkDeviceExSupport(device, desc._enabledExtensions); // 检查扩展支持
}

void KPhysicalDevice::_findMaxUsableSampleCount()
{
	// 获取物理设备属性
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);

	// 获取颜色和深度缓冲区的采样数
	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
		physicalDeviceProperties.limits.framebufferDepthSampleCounts;

	// 选择最大的可用采样数
	if (counts & VK_SAMPLE_COUNT_64_BIT)
		m_msaaSamples = VK_SAMPLE_COUNT_64_BIT;
	if (counts & VK_SAMPLE_COUNT_32_BIT)
		m_msaaSamples = VK_SAMPLE_COUNT_32_BIT;
	if (counts & VK_SAMPLE_COUNT_16_BIT)
		m_msaaSamples = VK_SAMPLE_COUNT_16_BIT;
	if (counts & VK_SAMPLE_COUNT_8_BIT)
		m_msaaSamples = VK_SAMPLE_COUNT_8_BIT;
	if (counts & VK_SAMPLE_COUNT_4_BIT)
		m_msaaSamples = VK_SAMPLE_COUNT_4_BIT;
	if (counts & VK_SAMPLE_COUNT_2_BIT)
		m_msaaSamples = VK_SAMPLE_COUNT_2_BIT;
}

bool KPhysicalDevice::_findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, const KDeviceCreateDesc& desc)
{
	// 获取设备队列族属性
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
	if (queueFamilies.empty())
		return false;

	// 查找需要的队列族
	// VK_QUEUE_GRAPHICS_BIT：支持图形渲染
	// VK_QUEUE_COMPUTE_BIT：支持计算着色器
	// VK_QUEUE_TRANSFER_BIT：支持传输操作
	// VK_QUEUE_SPARSE_BINDING_BIT：支持稀疏资源绑定
	for (uint32_t index = 0; index < queueFamilyCount; ++index)
	{
		const auto& props = queueFamilies[index];
		if (props.queueCount == 0)
			continue;

		for (const auto& req : desc._queueRequests)
		{
			switch (req._type)
			{
			case QueueType::Graphics:
			{
				if ((props.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
					req._count <= props.queueCount &&
					m_queueFamilies._graphicsFamily < 0)
					m_queueFamilies._graphicsFamily = index;
				break;
			}
			case QueueType::Present:
			{
				VkBool32 presentSupport = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(
					device, index, surface, &presentSupport);

				if (presentSupport &&
					req._count <= props.queueCount &&
					m_queueFamilies._presentFamily < 0)
					m_queueFamilies._presentFamily = index;
				break;
			}
			default:
				break;
			}
		}

		if (m_queueFamilies.IsComplete())
			return true;
	}

	return false;
}

bool KPhysicalDevice::_checkDeviceExSupport(VkPhysicalDevice device, const std::vector<const char*> deviceReqExts)
{
	uint32_t count = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

	std::vector<VkExtensionProperties> exts(count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &count, exts.data());

	// 检查每个需要的扩展是否在可用扩展列表中
	for (const auto& name : deviceReqExts)
	{
		bool found = false;
		for (const auto& ext : exts)
		{
			if (strcmp(ext.extensionName, name) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
			return false;
	}

	return true;
}
