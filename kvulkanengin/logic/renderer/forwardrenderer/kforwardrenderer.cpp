#include "kforwardrenderer.h"

#include "core/renderPass/krenderpass.h"
#include "core/configs/renderpassdesc.h"
#include "core/configs/pipelinecreatedesc.h"

#include <assert.h>
#include <filesystem>
#include <fstream>
#include <string>

static bool LoadSpirvFile(
	const char* path,
	std::vector<uint32_t>& outSpirv)
{
	std::ifstream file(path, std::ios::ate | std::ios::binary);
	if (!file.is_open())
		return false;

	std::streamsize size = file.tellg();
	if (size <= 0 || (size % 4) != 0)
		return false;

	outSpirv.resize(static_cast<size_t>(size / 4));
	file.seekg(0);
	file.read(reinterpret_cast<char*>(outSpirv.data()), size);
	return true;
}

bool KForwardRenderer::Init(KLogicContext& ctx)
{
	if (!_createRenderPass(ctx))
		return false;
	if (!_createPipeline(ctx))
		return false;

	if (!_createFramebuffers(ctx))
		return false;

	return true;
}

void KForwardRenderer::Destroy()
{
	// 注意：此方法不等待设备空闲，应该在外部确保设备已空闲后再调用
	// 因为 framebuffer 和 renderpass 可能正在被命令缓冲区使用
	_destroyFramebuffers();
	_destroyPipeline();
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
	_destroyPipeline();
	m_renderPass.Destroy();

	if (!_createRenderPass(ctx))
		return;
	if (!_createPipeline(ctx))
		return;
	if (!_createFramebuffers(ctx))
		return;
}

bool KForwardRenderer::_createRenderPass(KLogicContext& ctx)
{
	auto& device = ctx.getCoreCtx()->GetDevice();
	auto* swapchain = ctx.GetSwapchain();
	
	if (!swapchain)
		return false;

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

	return m_renderPass.Init(device.GetHandle(), desc);
}

bool KForwardRenderer::_createPipeline(KLogicContext& ctx)
{
	auto* swapchain = ctx.GetSwapchain();
	if (!swapchain)
		return false;

	auto& device = ctx.getCoreCtx()->GetDevice();

	constexpr const char* vertPath = "assets/shaders/forward.vert.spv";
	constexpr const char* fragPath = "assets/shaders/forward.frag.spv";

	std::vector<uint32_t> vertSpv;
	std::vector<uint32_t> fragSpv;

	if (!LoadSpirvFile(vertPath, vertSpv)
		|| !LoadSpirvFile(fragPath, fragSpv))
		return false;

	if (!m_vertShader.Init(device.GetHandle(), vertSpv)
		|| !m_fragShader.Init(device.GetHandle(), fragSpv))
	{
		_destroyPipeline();
		return false;
	}

	if (!m_pipelineLayout.Init(device.GetHandle(), {}))
	{
		_destroyPipeline();
		return false;
	}

	KPipelineCreateDesc pipelineDesc{};
	pipelineDesc.pipelineLayout = m_pipelineLayout.GetHandle();
	pipelineDesc.renderPass = m_renderPass.GetHandle();
	pipelineDesc.subpass = 0;

	// Shader stages
	PipelineShaderStageDesc vs{};
	vs.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vs.module = m_vertShader.GetHandle();
	vs.entry = "main";
	PipelineShaderStageDesc fs{};
	fs.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fs.module = m_fragShader.GetHandle();
	fs.entry = "main";
	pipelineDesc.shaders = { vs, fs };

	// Vertex input: none (use gl_VertexIndex in shader)
	pipelineDesc.vertexInput = {};

	// Input assembly
	pipelineDesc.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineDesc.inputAssembly.primitiveRestart = false;

	// Viewport / Scissor
	auto extent = swapchain->GetExtent();
	pipelineDesc.viewport.dynamic = false;
	pipelineDesc.viewport.viewport = {
		0.0f, 0.0f,
		static_cast<float>(extent.width),
		static_cast<float>(extent.height),
		0.0f, 1.0f
	};
	pipelineDesc.viewport.scissor = { {0, 0}, extent };

	// Raster
	pipelineDesc.raster.polygonMode = VK_POLYGON_MODE_FILL;
	pipelineDesc.raster.cullMode = VK_CULL_MODE_BACK_BIT;
	pipelineDesc.raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pipelineDesc.raster.lineWidth = 1.0f;

	// Depth
	pipelineDesc.depth.enableDepthTest = false;
	pipelineDesc.depth.enableDepthWrite = false;
	pipelineDesc.depth.depthCompareOp = VK_COMPARE_OP_LESS;

	// Blend (single color attachment)
	PipelineBlendAttachmentDesc blend{};
	blend.enableBlend = false;
	pipelineDesc.blend.attachments = { blend };

	if (!m_pipeline.Init(device.GetHandle(), pipelineDesc))
	{
		_destroyPipeline();
		return false;
	}

	return true;
}

void KForwardRenderer::_destroyPipeline()
{
	m_pipeline.Destroy();
	m_pipelineLayout.Destroy();
	m_vertShader.Destroy();
	m_fragShader.Destroy();
}


bool KForwardRenderer::_createFramebuffers(KLogicContext& ctx)
{
	auto swapchain = ctx.GetSwapchain();
	
	if (!swapchain)
		return false;

	auto extent = swapchain->GetExtent();
	auto& views = swapchain->GetImageViews();

	m_framebuffers.resize(views.size());

	for (size_t i = 0; i < views.size(); ++i)
	{
		std::vector<VkImageView> attachments = {views[i].GetHandle()};
		if (!m_framebuffers[i].Init(
			ctx.getCoreCtx()->GetDevice().GetHandle(),
			m_renderPass.GetHandle(),
			attachments,
			swapchain->GetExtent().width,
			swapchain->GetExtent().height))
		{
			for (auto& fb : m_framebuffers)
				fb.Destroy();
			m_framebuffers.clear();
			return false;
		}
	}

	return true;
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
	vkCmdBindPipeline(cmd.GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.GetHandle());
	vkCmdDraw(cmd.GetHandle(), 3, 1, 0, 0);
	vkCmdEndRenderPass(cmd.GetHandle());

	cmd.End();
}

