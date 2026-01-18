#ifndef __SWAPCHAIN_DESC_H__
#define __SWAPCHAIN_DESC_H__

#include <vulkan.h>

struct SwapchainCreateDesc
{
	uint32_t _desiredImageCount = 2;
	VkFormat _preferredFormat = VK_FORMAT_B8G8R8A8_SRGB;
	VkColorSpaceKHR _preferredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	VkPresentModeKHR _preferredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	VkImageUsageFlags _imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	uint32_t width = 0;
	uint32_t height = 0;
	bool _allowRecreate = true;
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR _capabilities{};
	std::vector<VkSurfaceFormatKHR> _formats;
	std::vector<VkPresentModeKHR>   _presentModes;
};

#endif // !__SWAPCHAIN_DESC_H__
