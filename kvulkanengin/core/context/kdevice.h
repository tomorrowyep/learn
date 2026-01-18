#ifndef __KDEVICE_H__
#define __KDEVICE_H__

#include <vulkan.h>
#include <unordered_map>

#include "kphysicaldevice.h"
#include "devicedesc.h"
#include "commondesc.h"

class KDevice
{
public:
	KDevice() = default;
	~KDevice();

	// 禁止拷贝
	KDevice(const KDevice&) = delete;
	KDevice& operator=(const KDevice&) = delete;

	bool Create(const KPhysicalDevice& physicalDevice, const KDeviceCreateDesc& desc);
	void Destroy();

	void SubmitGraphics(const VkSubmitInfo& submit, VkFence fence = VK_NULL_HANDLE) const;

	VkDevice GetHandle() const { return m_logicDevice; }

	void WaitIdle() { vkDeviceWaitIdle(m_logicDevice); }

	const QueueFamilyIndexs& GetQueueFamilies() const { return m_queueFamiliesIdxs; }
	VkSampleCountFlagBits GetMaxUsableSampleCount() const { return m_msaaSamples; }

	VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
	VkQueue GetPresentQueue()  const { return m_presentQueue; }

private:
	struct QueueFamilyBuildInfo
	{
		uint32_t _familyIndex = 0;
		uint32_t _requiredCount = 0;
		std::vector<float> _priorities;
	};

	using QueueFamilysMap = std::unordered_map<uint32_t, KDevice::QueueFamilyBuildInfo>;
	QueueFamilysMap _mergeQueueFamilyInfo(const KDeviceCreateDesc& desc) const;

private:
	VkDevice m_logicDevice{ VK_NULL_HANDLE };

	QueueFamilyIndexs m_queueFamiliesIdxs{};
	VkQueue  m_graphicsQueue{ VK_NULL_HANDLE };
	VkQueue  m_presentQueue{ VK_NULL_HANDLE };
	VkSampleCountFlagBits m_msaaSamples{ VK_SAMPLE_COUNT_1_BIT };
};

#endif // !__KDEVICE_H__
