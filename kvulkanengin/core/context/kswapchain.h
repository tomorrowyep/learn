#ifndef _KSWAPCHAIN_H__
#define _KSWAPCHAIN_H__

#include <vulkan.h>
#include <vector>

#include "swapchaindesc.h"
#include "objects/kimageview.h"

class KDevice;

class KSwapchain
{
public:
	KSwapchain() = default;
	~KSwapchain();

	KSwapchain(const KSwapchain&) = delete;
	KSwapchain& operator=(const KSwapchain&) = delete;

	bool Create(
		const KDevice& device,
		VkSurfaceKHR surface,
		const SwapChainSupportDetails& support,
		const SwapchainCreateDesc& desc,
		VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);

	void Recreate(
		const KDevice& device,
		VkSurfaceKHR surface,
		const SwapChainSupportDetails& support,
		const SwapchainCreateDesc& desc);

	VkResult AcquireNextImage(VkSemaphore semaphore, uint32_t* pimageIndex);
	VkResult Present(VkSemaphore waitSemaphore, uint32_t imageIndex);

	void Destroy();

	VkSwapchainKHR GetHandle() const { return m_swapchain; }
	VkFormat       GetImageFormat() const { return m_surfaceFormat.format; }
	VkExtent2D     GetExtent() const { return m_extent; }

	uint32_t GetImageCount() const { return static_cast<uint32_t>(m_images.size()); }

	const std::vector<KImageView>& GetImageViews() const { return m_imageViews; }

private:
	// 选择交换链表面格式
	VkSurfaceFormatKHR _chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	// 选择交换链显示模式
	VkPresentModeKHR _chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

	void _createImageView();

private:
	VkDevice                  m_device{ VK_NULL_HANDLE };
	VkSwapchainKHR            m_swapchain{ VK_NULL_HANDLE };
	VkSurfaceFormatKHR        m_surfaceFormat{ VK_FORMAT_UNDEFINED };
	VkExtent2D                m_extent{};
	VkQueue m_presentQueue = VK_NULL_HANDLE;

	std::vector<VkImage>      m_images;
	std::vector<KImageView>  m_imageViews;
};

#endif // !_KSWAPCHAIN_H__
