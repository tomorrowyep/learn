#include "kswapchain.h"
#include "kdevice.h"
#include <cassert>
#include <algorithm>

KSwapchain::~KSwapchain()
{
	Destroy();
}

bool KSwapchain::Create(const KDevice& device,
	VkSurfaceKHR surface,
	const SwapChainSupportDetails& support,
	const SwapchainCreateDesc& desc,
	VkSwapchainKHR oldSwapchain)
{
	m_device = device.GetHandle();
	m_presentQueue = device.GetPresentQueue();
	m_surfaceFormat = _chooseSwapSurfaceFormat(support._formats);
	VkPresentModeKHR presentMode = _chooseSwapPresentMode(support._presentModes);
	m_extent.width = std::clamp(desc.width, support._capabilities.minImageExtent.width, support._capabilities.maxImageExtent.width);
	m_extent.height = std::clamp(desc.height, support._capabilities.minImageExtent.height, support._capabilities.maxImageExtent.height);

	// 交换链图像数量
	// maxImageCount的值为0表明，只要内存可以满足，我们可以使用任意数量的图像
	uint32_t imageCount = support._capabilities.minImageCount + 1;
	if (support._capabilities.maxImageCount > 0
		&& imageCount > support._capabilities.maxImageCount)
		imageCount = support._capabilities.maxImageCount;

	// 交换链创建信息
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface; // 交换链表面
	createInfo.minImageCount = imageCount; // 交换链图像数量
	createInfo.imageFormat = m_surfaceFormat.format; // 交换链表面格式
	createInfo.imageColorSpace = m_surfaceFormat.colorSpace; // 交换链颜色空间
	createInfo.imageExtent = m_extent; // 交换链图像范围
	createInfo.imageArrayLayers = 1; // 图像层数
	//交换链图像	VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
	//采样纹理	VK_IMAGE_USAGE_SAMPLED_BIT
	//上传纹理数据	VK_IMAGE_USAGE_TRANSFER_DST_BIT
	//渲染输出截图	VK_IMAGE_USAGE_TRANSFER_SRC_BIT
	//离屏渲染 + 导出图像	VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | TRANSFER_SRC_BIT
	createInfo.imageUsage = desc._imageUsage;// 图像用法（颜色附件）
	createInfo.preTransform = support._capabilities.currentTransform; // 交换链转换
	//VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR	不透明合成，忽略 alpha，窗口完全不透明 默认、安全
	//VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR	alpha 已经 预乘（Premultiplied Alpha）	用于支持真实半透明窗口
	//VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR	alpha 是 未预乘，系统会乘上 alpha	同上，窗口半透明
	//VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR	从 平台窗口系统继承合成方式	有时用于特殊平台行为
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // 复合Alpha
	createInfo.presentMode = presentMode; // 交换链显示模式
	createInfo.clipped = VK_TRUE; // 是否裁剪
	createInfo.oldSwapchain = oldSwapchain; // 旧交换链
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // 图像共享模式
	createInfo.queueFamilyIndexCount = 0; // 队列族索引数量
	createInfo.pQueueFamilyIndices = nullptr; // 队列族索引

	// 交换链创建信息
	if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS)
		return false;

	// 获取交换链图像
	uint32_t swapChainImageCount = 0;
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapChainImageCount, nullptr);
	m_images.resize(swapChainImageCount);
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapChainImageCount, m_images.data());

	_createImageView();

	return true;
}

void KSwapchain::Recreate(const KDevice& device,
	VkSurfaceKHR surface,
	const SwapChainSupportDetails& support,
	const SwapchainCreateDesc& desc)
{
	/*
	* 重建逻辑（Recreate）
	处理窗口大小变化
	销毁原 framebuffer / depth / MSAA / swapchain
	创建新的 swapchain 并重新生成派生资源

	派生资源管理
	ImageViews
	Depth / MSAA attachments
	Framebuffers

	依赖注入
	Renderer / RenderPass 不再创建或持有 swapchain 派生资源
	直接引用 KSwapchain 提供的接口
	Per-frame 资源同步
	例如 fences / semaphores / command buffers
	*/
	assert(m_device != VK_NULL_HANDLE);

	// 1. 等待设备空闲（Core 层允许这么做）
	vkDeviceWaitIdle(m_device);

	// 2. 保存旧 swapchain
	VkSwapchainKHR oldSwapchain = m_swapchain;

	// 3. 销毁旧 image views
	m_images.clear();

	// 4. 重新创建 swapchain
	bool ret = Create(
		device,
		surface,
		support,
		desc,
		oldSwapchain);

	assert(ret && "Failed to recreate swapchain");

	// 5. 销毁旧 swapchain
	if (oldSwapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(m_device, oldSwapchain, nullptr);
	}
}

VkResult KSwapchain::AcquireNextImage(VkSemaphore semaphore, uint32_t* pimageIndex)
{
	return vkAcquireNextImageKHR(m_device,
		m_swapchain,
		UINT64_MAX,
		semaphore,
		VK_NULL_HANDLE,
		pimageIndex);
}

VkResult KSwapchain::Present(VkSemaphore waitSemaphore, uint32_t imageIndex)
{
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &waitSemaphore;

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_swapchain;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr;

	return vkQueuePresentKHR(m_presentQueue, &presentInfo);
}

void KSwapchain::Destroy()
{
	// 等待设备空闲，确保所有命令执行完成
	if (m_device != VK_NULL_HANDLE)
	{
		vkDeviceWaitIdle(m_device);
	}

	// 先销毁 image views（需要在设备有效时销毁）
	m_imageViews.clear();
	m_images.clear();

	// 然后销毁 swapchain
	if (m_swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
		m_swapchain = VK_NULL_HANDLE;
	}

	m_device = VK_NULL_HANDLE;
}

VkSurfaceFormatKHR KSwapchain::_chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	// 选择表面格式
	// VK_FORMAT_B8G8R8A8_UNORM：表示 BGRA 排列，被广泛使用，且支持度高
	// VK_COLOR_SPACE_SRGB_NONLINEAR_KHR：sRGB颜色空间
	// VK_PRESENT_MODE_FIFO_KHR：先进先出模式

	// 判断是否 format == VK_FORMAT_UNDEFINED	如果是，说明可以自由选择格式
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	// 遍历所有表面格式
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM
			&& availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	// 如果没有找到合适的格式，返回第一个格式
	return availableFormats[0];
}

VkPresentModeKHR KSwapchain::_chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	// VK_PRESENT_MODE_IMMEDIATE_KHR：立即模式，图像会立即显示
	// VK_PRESENT_MODE_MAILBOX_KHR：邮箱模式，类似于三重缓冲，适合高性能渲染，延迟比较低，会丢弃旧帧
	// VK_PRESENT_MODE_FIFO_KHR：先进先出模式，图像会在队列中排队，类似两重缓冲，有一定延迟
	// VK_PRESENT_MODE_FIFO_RELAXED_KHR：放松的先进先出模式，类似于先进先出模式，但允许提前显示
	// VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR：共享需求刷新模式，适合多窗口渲染
	// VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR：共享连续刷新模式，适合多窗口渲染

	// 默认是 FIFO 模式：保证所有平台都支持
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& mode : availablePresentModes)
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode; // 优先返回性能最佳的 MAILBOX 模式
		else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			bestMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // 作为次选
	}

	return bestMode; // 没有 MAILBOX 就用次优（IMMEDIATE 或 FIFO）
}

void KSwapchain::_createImageView()
{
	m_imageViews.clear();
	m_imageViews.reserve(m_images.size());
	for (VkImage image : m_images)
	{
		KImageView view;
		bool ok = view.Init(m_device, image, m_surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
		assert(ok);
		m_imageViews.emplace_back(std::move(view));
	}
}
