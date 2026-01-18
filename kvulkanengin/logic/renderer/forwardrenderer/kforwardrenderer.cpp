#include "kforwardrenderer.h"

#include "core/renderPass/krenderpass.h"
#include "core/configs/renderpassdesc.h"

#include <assert.h>

bool KForwardRenderer::Init(KLogicContext& ctx)
{
	_createRenderPass(ctx);
	_createFramebuffers(ctx);
	return true;
}

void KForwardRenderer::Destroy()
{
	// 注意：此方法不等待设备空闲，应该在外部确保设备已空闲后再调用
	// 因为 framebuffer 和 renderpass 可能正在被命令缓冲区使用
	_destroyFramebuffers();
	m_renderPass.Destroy();
}

void KForwardRenderer::_destroyFramebuffers()
{
	for (auto& fb : m_framebuffers)
		fb.Destroy();

	m_framebuffers.clear();
}

void KForwardRenderer::OnSwapchainRecreated(KLogicContext& ctx)
{
	_destroyFramebuffers();
	_createFramebuffers(ctx);
}

void KForwardRenderer::_createRenderPass(KLogicContext& ctx)
{
	auto& device = ctx.getCoreCtx()->GetDevice();
	auto* swapchain = ctx.GetSwapchain();
	
	if (!swapchain)
		return;

	// ---------- Attachment ----------
	KAttachmentDesc colorAttachment{};
	colorAttachment.format = swapchain->GetImageFormat();
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// ---------- Subpass ----------
	KSubpassDesc subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	KSubpassAttachmentRef colorRef{};
	colorRef.attachmentIndex = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	subpass.colorAttachments.push_back(colorRef);

	// ---------- Dependency ----------
	KSubpassDependencyDesc dep{};
	dep.srcSubpass = VK_SUBPASS_EXTERNAL;
	dep.dstSubpass = 0;

	dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	dep.srcAccessMask = 0;
	dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// ---------- RenderPass Desc ----------
	KRenderPassCreateDesc desc{};
	desc.attachments.push_back(colorAttachment);
	desc.subpasses.push_back(subpass);
	desc.dependencies.push_back(dep);

	m_renderPass.Init(device.GetHandle(), desc);
}


void KForwardRenderer::_createFramebuffers(KLogicContext& ctx)
{
	auto swapchain = ctx.GetSwapchain();
	
	if (!swapchain)
		return;

	auto extent = swapchain->GetExtent();
	auto& views = swapchain->GetImageViews();

	m_framebuffers.resize(views.size());

	for (size_t i = 0; i < views.size(); ++i)
	{
		std::vector<VkImageView> attachments = {views[i].GetHandle()};
		m_framebuffers[i].Init(
			ctx.getCoreCtx()->GetDevice().GetHandle(),
			m_renderPass.GetHandle(),
			attachments,
			swapchain->GetExtent().width,
			swapchain->GetExtent().height);
	}
}

void KForwardRenderer::Record(
	KLogicContext& ctx,
	KFrameResources& frame,
	uint32_t imageIndex)
{
	auto* swapchain = ctx.GetSwapchain();
	if (!swapchain || imageIndex >= m_framebuffers.size())
		return;

	auto& cmd = frame.GetCommandBuffer();

	// Renderer 负责管理命令缓冲区的记录生命周期
	cmd.Begin();

	VkClearValue clear{};
	clear.color = { {0.2f, 0.3f, 0.5f, 1.0f} };

	VkRenderPassBeginInfo rpBegin{};
	rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBegin.renderPass = m_renderPass.GetHandle();
	rpBegin.framebuffer = m_framebuffers[imageIndex].GetHandle();
	rpBegin.renderArea.offset = { 0, 0 };
	rpBegin.renderArea.extent = swapchain->GetExtent();
	rpBegin.clearValueCount = 1;
	rpBegin.pClearValues = &clear;

	vkCmdBeginRenderPass(cmd.GetHandle(), &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(cmd.GetHandle());

	cmd.End();
}

