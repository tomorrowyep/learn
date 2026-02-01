#include "forwardpass.h"
#include "../resource/resourcemanager.h"
#include "../resource/mesh.h"
#include "../resource/material.h"
#include "core/configs/renderpassdesc.h"
#include "core/configs/pipelinecreatedesc.h"
#include <array>
#include <fstream>
#include <iostream>

namespace kEngine
{

static std::vector<uint32_t> loadSPIRV(const std::string& path)
{
	std::ifstream file(path, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "[ForwardPass] Failed to open shader: " << path << std::endl;
		return {};
	}

	size_t size = static_cast<size_t>(file.tellg());
	std::vector<uint32_t> buffer(size / sizeof(uint32_t));
	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), size);
	return buffer;
}

bool ForwardPass::init(KCoreContext& ctx, KSwapchain& swapchain, ResourceManager& resources)
{
	m_device = ctx.GetDevice().GetHandle();
	VkPhysicalDevice physDevice = ctx.GetPhysicalDevice().GetHandle();
	m_extent = swapchain.GetExtent();
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

	if (!createRenderPass(m_device, swapchain.GetImageFormat(), depthFormat))
		return false;

	if (!createDepthResources(m_device, physDevice, m_extent.width, m_extent.height))
		return false;

	if (!createFramebuffers(m_device, swapchain))
		return false;

	if (!createCameraDescriptorSetLayout(m_device))
		return false;

	if (!createPipeline(m_device, m_extent, resources.getMaterialDescriptorSetLayout()))
		return false;

	if (!createCameraUBO(m_device, physDevice))
		return false;

	if (!createCameraDescriptorPool(m_device))
		return false;

	if (!createCameraDescriptorSet(m_device))
		return false;

	m_initialized = true;
	std::cout << "[ForwardPass] Initialized" << std::endl;
	return true;
}

void ForwardPass::destroy()
{
	if (!m_initialized)
		return;

	m_cameraUBO.Destroy();
	m_cameraDescriptorPool.Destroy();
	m_cameraDescriptorSetLayout.Destroy();
	m_pipeline.Destroy();
	m_pipelineLayout.Destroy();
	m_vertShader.Destroy();
	m_fragShader.Destroy();
	destroyFramebuffers();
	destroyDepthResources();
	m_renderPass.Destroy();
	m_initialized = false;
}

void ForwardPass::onResize(KCoreContext& ctx, KSwapchain& swapchain)
{
	m_extent = swapchain.GetExtent();
	destroyFramebuffers();
	destroyDepthResources();

	VkPhysicalDevice physDevice = ctx.GetPhysicalDevice().GetHandle();
	createDepthResources(m_device, physDevice, m_extent.width, m_extent.height);
	createFramebuffers(m_device, swapchain);
}

void ForwardPass::render(VkCommandBuffer cmd, uint32_t imageIndex, const RenderScene& renderScene, ResourceManager& resources)
{
	if (!m_initialized || !renderScene.hasCamera)
		return;

	// Update camera UBO
	void* data = m_cameraUBO.Map();
	memcpy(data, &renderScene.cameraData, sizeof(CameraUBO));
	m_cameraUBO.Unmap();

	// Begin render pass
	VkRenderPassBeginInfo rpBegin{};
	rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpBegin.renderPass = m_renderPass.GetHandle();
	rpBegin.framebuffer = m_framebuffers[imageIndex].GetHandle();
	rpBegin.renderArea.extent = m_extent;

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = {{0.02f, 0.02f, 0.05f, 1.0f}};
	clearValues[1].depthStencil = {1.0f, 0};
	rpBegin.clearValueCount = static_cast<uint32_t>(clearValues.size());
	rpBegin.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(cmd, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.GetHandle());

	// Set dynamic viewport and scissor
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_extent.width);
	viewport.height = static_cast<float>(m_extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = m_extent;
	vkCmdSetScissor(cmd, 0, 1, &scissor);

	// Bind camera descriptor set
	VkDescriptorSet cameraSet = m_cameraDescriptorSet.GetHandle();
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout.GetHandle(), 0, 1, &cameraSet, 0, nullptr);

	// Draw opaque items
	for (const auto& item : renderScene.opaqueItems)
	{
		Mesh* mesh = resources.getMesh(item.meshID);
		Material* material = resources.getMaterial(item.materialID);

		if (!mesh || !mesh->isUploaded())
			continue;

		if (!material || !material->isUploaded())
		{
			material = resources.getMaterial(resources.getDefaultMaterial());
			if (!material)
				continue;
		}

		// Bind material descriptor set
		VkDescriptorSet materialSet = material->descriptorSet.GetHandle();
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout.GetHandle(), 1, 1, &materialSet, 0, nullptr);

		// Push model matrix
		vkCmdPushConstants(cmd, m_pipelineLayout.GetHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &item.transform);

		// Bind vertex and index buffers
		VkBuffer vb = mesh->vertexBuffer.GetHandle();
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &offset);
		vkCmdBindIndexBuffer(cmd, mesh->indexBuffer.GetHandle(), 0, VK_INDEX_TYPE_UINT32);

		// Draw
		vkCmdDrawIndexed(cmd, mesh->getIndexCount(), 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(cmd);
}

bool ForwardPass::createRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat)
{
	KRenderPassCreateDesc desc;

	// Color attachment
	KAttachmentDesc colorAtt{};
	colorAtt.format = colorFormat;
	colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	desc.attachments.push_back(colorAtt);

	// Depth attachment
	KAttachmentDesc depthAtt{};
	depthAtt.format = depthFormat;
	depthAtt.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAtt.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAtt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	desc.attachments.push_back(depthAtt);

	// Subpass
	KSubpassDesc subpass;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachments.push_back({0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
	subpass.depthStencilAttachment = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
	desc.subpasses.push_back(subpass);

	// Dependency
	KSubpassDependencyDesc dep{};
	dep.srcSubpass = VK_SUBPASS_EXTERNAL;
	dep.dstSubpass = 0;
	dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dep.srcAccessMask = 0;
	dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	desc.dependencies.push_back(dep);

	return m_renderPass.Init(device, desc);
}

bool ForwardPass::createDepthResources(VkDevice device, VkPhysicalDevice physDevice, uint32_t w, uint32_t h)
{
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

	if (!m_depthImage.Init(device, physDevice, w, h, depthFormat,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
	{
		return false;
	}

	if (!m_depthImageView.Init(device, m_depthImage.GetHandle(), depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT))
	{
		return false;
	}

	return true;
}

void ForwardPass::destroyDepthResources()
{
	m_depthImageView.Destroy();
	m_depthImage.Destroy();
}

bool ForwardPass::createFramebuffers(VkDevice device, KSwapchain& swapchain)
{
	m_framebuffers.resize(swapchain.GetImageCount());

	for (size_t i = 0; i < swapchain.GetImageCount(); ++i)
	{
		std::vector<VkImageView> attachments =
		{
			swapchain.GetImageViews()[i].GetHandle(),
			m_depthImageView.GetHandle()
		};

		if (!m_framebuffers[i].Init(device, m_renderPass.GetHandle(), attachments, m_extent.width, m_extent.height))
			return false;
	}

	return true;
}

void ForwardPass::destroyFramebuffers()
{
	for (auto& fb : m_framebuffers)
	{
		fb.Destroy();
	}
	m_framebuffers.clear();
}

bool ForwardPass::createCameraDescriptorSetLayout(VkDevice device)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings =
	{
		{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
	};

	return m_cameraDescriptorSetLayout.Init(device, bindings);
}

bool ForwardPass::createPipeline(VkDevice device, VkExtent2D extent, VkDescriptorSetLayout materialLayout)
{
	auto vertSPV = loadSPIRV("assets/shaders/forward.vert.spv");
	auto fragSPV = loadSPIRV("assets/shaders/forward.frag.spv");

	if (vertSPV.empty() || fragSPV.empty())
		return false;

	if (!m_vertShader.Init(device, vertSPV))
		return false;

	if (!m_fragShader.Init(device, fragSPV))
		return false;

	// Pipeline layout
	VkPushConstantRange pushConst{};
	pushConst.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConst.offset = 0;
	pushConst.size = sizeof(glm::mat4);

	std::vector<VkDescriptorSetLayout> layouts =
	{
		m_cameraDescriptorSetLayout.GetHandle(),
		materialLayout
	};

	m_pipelineLayout.Init(device, layouts, {pushConst});

	// Pipeline create desc
	KPipelineCreateDesc desc;
	desc.pipelineLayout = m_pipelineLayout.GetHandle();
	desc.renderPass = m_renderPass.GetHandle();
	desc.subpass = 0;

	// Shaders
	desc.shaders.push_back({VK_SHADER_STAGE_VERTEX_BIT, m_vertShader.GetHandle(), "main"});
	desc.shaders.push_back({VK_SHADER_STAGE_FRAGMENT_BIT, m_fragShader.GetHandle(), "main"});

	// Vertex input
	desc.vertexInput.bindings.push_back(Vertex::getBindingDescription());
	for (auto& attr : Vertex::getAttributeDescriptions())
	{
		desc.vertexInput.attributes.push_back(attr);
	}

	// Input assembly
	desc.inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// Viewport - dynamic
	desc.viewport.dynamic = true;

	// Rasterization
	desc.raster.polygonMode = VK_POLYGON_MODE_FILL;
	desc.raster.cullMode = VK_CULL_MODE_BACK_BIT;
	desc.raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	desc.raster.lineWidth = 1.0f;

	// Depth
	desc.depth.enableDepthTest = true;
	desc.depth.enableDepthWrite = true;
	desc.depth.depthCompareOp = VK_COMPARE_OP_LESS;

	// Blend
	PipelineBlendAttachmentDesc blend;
	blend.enableBlend = false;
	desc.blend.attachments.push_back(blend);

	return m_pipeline.Init(device, desc);
}

bool ForwardPass::createCameraUBO(VkDevice device, VkPhysicalDevice physDevice)
{
	return m_cameraUBO.Init(device, physDevice, sizeof(CameraUBO),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

bool ForwardPass::createCameraDescriptorPool(VkDevice device)
{
	std::vector<VkDescriptorPoolSize> sizes =
	{
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}
	};

	return m_cameraDescriptorPool.Init(device, sizes, 1);
}

bool ForwardPass::createCameraDescriptorSet(VkDevice device)
{
	if (!m_cameraDescriptorSet.Allocate(device, m_cameraDescriptorPool.GetHandle(), m_cameraDescriptorSetLayout.GetHandle()))
		return false;

	VkDescriptorBufferInfo bufInfo{};
	bufInfo.buffer = m_cameraUBO.GetHandle();
	bufInfo.offset = 0;
	bufInfo.range = sizeof(CameraUBO);

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = m_cameraDescriptorSet.GetHandle();
	write.dstBinding = 0;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.descriptorCount = 1;
	write.pBufferInfo = &bufInfo;

	m_cameraDescriptorSet.Update({write});
	return true;
}

} // namespace kEngine
