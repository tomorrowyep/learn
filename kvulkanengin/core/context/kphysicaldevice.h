#ifndef __KPHYSICALDEVICE_H__
#define __KPHYSICALDEVICE_H__

#include <vulkan.h>
#include <vector>

#include "devicedesc.h"
#include "swapchaindesc.h"

class KPhysicalDevice
{
public:
	KPhysicalDevice() = default;
	~KPhysicalDevice() = default;

	// 禁止拷贝
	KPhysicalDevice(const KPhysicalDevice&) = delete;
	KPhysicalDevice& operator=(const KPhysicalDevice&) = delete;

	bool Pick(VkInstance instance, VkSurfaceKHR surface, const KDeviceCreateDesc& desc);

	bool QuerySwapChainSupport(VkSurfaceKHR surface, SwapChainSupportDetails& details) const;

	VkPhysicalDevice GetHandle() const { return m_physicalDevice; }
	const QueueFamilyIndexs& GetQueueFamilies() const { return m_queueFamilies; }
	VkSampleCountFlagBits GetMaxUsableSampleCount() const { return m_msaaSamples; }

private:
	bool _isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const KDeviceCreateDesc& desc);
	void _findMaxUsableSampleCount();
	bool _findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, const KDeviceCreateDesc& desc);
	bool _checkDeviceExSupport(VkPhysicalDevice device, const std::vector<const char*> deviceReqExts);

	void _reset()
	{
		m_physicalDevice = VK_NULL_HANDLE;
		m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
		m_queueFamilies = {};
	}

private:
	VkPhysicalDevice   m_physicalDevice{ VK_NULL_HANDLE };
	VkSampleCountFlagBits m_msaaSamples{ VK_SAMPLE_COUNT_1_BIT };
	QueueFamilyIndexs m_queueFamilies{};
};

#endif // !__KPHYSICALDEVICE_H__
