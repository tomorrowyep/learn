#ifndef __DEVICE_DESC_H__
#define __DEVICE_DESC_H__

#include <vulkan.h>
#include <vector>

#include "commondesc.h"

struct QueueRequest
{
	QueueType _type;
	uint32_t  _count = 1;
	float     _priority = 1.0f;
};

struct DeviceFeatureSet
{
	bool _samplerAnisotropy = false;
	bool _sampleRateShading = false;
	bool _fillModeNonSolid = false;

	/*VkPhysicalDeviceVulkan11Features _features11{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
	VkPhysicalDeviceVulkan12Features _features12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	VkPhysicalDeviceVulkan13Features _features13{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };*/
};

// 逻辑设备创建描述
struct KDeviceCreateDesc
{
	std::vector<QueueRequest>      _queueRequests;
	DeviceFeatureSet               _enabledFeatures;
	std::vector<const char*>       _enabledExtensions;
};

#endif // !__DEVICE_DESC_H__
