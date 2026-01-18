#include "klogiccontext.h"
#include "renderer/include/irenderer.h"
#include "platform/include/ikwindow.h"

#include "core/context/kcorecontext.h"
#include "core/context/kswapchain.h"
#include "core/commands/kcommandbuffer.h"
#include "core/sync/kfence.h"
#include "core/sync/ksemaphore.h"

bool KLogicContext::Init(KCoreContext& core, const SwapchainCreateDesc& desc)
{
	m_core = &core;
	m_currentFrame = 0;
	m_swapChainDesc = desc;

	SwapChainSupportDetails support;
	m_core->GetPhysicalDevice().QuerySwapChainSupport(m_core->GetSurface().GetHandle(), support);

	if (!m_swapchain.Create(
		m_core->GetDevice(),
		m_core->GetSurface().GetHandle(),
		support,
		desc))
	{
		return false;
	}

	m_maxFrames = m_swapchain.GetImageCount();

	// 创建 FrameResources
	m_frames.resize(m_maxFrames);
	for (uint32_t i = 0; i < m_maxFrames; ++i)
	{
		if (!m_frames[i].Init(m_core->GetDevice()))
			return false;
	}

	return true;
}

void KLogicContext::Destroy()
{
	if (!m_core)
		return;

	// 等待设备空闲，确保所有命令执行完成
	m_core->GetDevice().WaitIdle();

	// 先销毁 frames（可能依赖 swapchain）
	for (auto& frame : m_frames)
		frame.Destroy();

	m_frames.clear();

	// 显式销毁 swapchain（需要在 device 有效时销毁）
	m_swapchain.Destroy();

	m_core = nullptr;
}

void KLogicContext::SetRenderer(IKRenderer* renderer)
{
	m_pRenderer = renderer;
}

void KLogicContext::SetWindow(IKWindow* pWin)
{
	m_pWin = pWin;
}

bool KLogicContext::BeginFrame()
{
	auto& frame = m_frames[m_currentFrame];

	// 1. 等待当前帧 fence
	frame.GetInFlightFence().Wait();
	frame.GetInFlightFence().Reset();

	// 2. Acquire Image
	VkResult res = m_swapchain.AcquireNextImage(
		*(frame.GetImageAvailableSemaphore().GetHandle()),
		&m_imageIndex);

	if (res == VK_ERROR_OUT_OF_DATE_KHR)
	{
		_recreateSwapchain();
		return false;
	}
	else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
	{
		return false;
	}

	// 3. 重置命令缓冲区（仅在成功获取图像后）
	// 注意：Begin() 和 End() 由 Renderer 的 Record() 函数管理
	frame.GetCommandBuffer().Reset();

	return true;
}

void KLogicContext::Render()
{
	if (!m_pRenderer)
		return;

	auto& frame = m_frames[m_currentFrame];

	m_pRenderer->Record(
		*this,
		frame,
		m_imageIndex);
}

void KLogicContext::EndFrame()
{
	auto& frame = m_frames[m_currentFrame];

	// 注意：命令缓冲区的 Begin() 和 End() 由 Renderer 的 Record() 函数管理
	VkCommandBuffer cmd = frame.GetCommandBuffer().GetHandle();

	// 提交命令
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submit{};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = frame.GetImageAvailableSemaphore().GetHandle();
	submit.pWaitDstStageMask = &waitStage;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmd;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = frame.GetRenderFinishedSemaphore().GetHandle();

	m_core->GetDevice().SubmitGraphics(submit, frame.GetInFlightFence().GetHandle());

	// Present
	VkResult res = m_swapchain.Present(
		*(frame.GetRenderFinishedSemaphore().GetHandle()),
		m_imageIndex);

	if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
	{
		_recreateSwapchain();
	}

	// 切换 Frame Index
	m_currentFrame = (m_currentFrame + 1) % m_maxFrames;
}

void KLogicContext::_recreateSwapchain()
{
	if (!m_core || !m_pWin)
		return;

	m_core->GetDevice().WaitIdle();

	SwapChainSupportDetails support;
	m_core->GetPhysicalDevice().QuerySwapChainSupport(m_core->GetSurface().GetHandle(), support);

	m_swapChainDesc.width = m_pWin->GetWidth();
	m_swapChainDesc.height = m_pWin->GetHeight();

	m_swapchain.Recreate(m_core->GetDevice(),
		m_core->GetSurface().GetHandle(),
		support,
		m_swapChainDesc);

	// 更新帧数量（如果 swapchain 图像数量改变）
	uint32_t newMaxFrames = m_swapchain.GetImageCount();
	if (newMaxFrames != m_maxFrames)
	{
		// 销毁旧的 frame resources
		for (auto& frame : m_frames)
			frame.Destroy();

		// 重新创建 frame resources
		m_maxFrames = newMaxFrames;
		m_frames.resize(m_maxFrames);
		for (uint32_t i = 0; i < m_maxFrames; ++i)
		{
			if (!m_frames[i].Init(m_core->GetDevice()))
				return;
		}

		// 重置当前帧索引
		m_currentFrame = 0;
	}

	if (m_pRenderer)
	{
		m_pRenderer->OnSwapchainRecreated(*this);
	}
}
