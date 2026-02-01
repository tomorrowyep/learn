#include "kpipeline.h"
#include <cassert>
#include <vector>

KPipeline::~KPipeline()
{
	Destroy();
}

bool KPipeline::Init(VkDevice device, const KPipelineCreateDesc& desc)
{
	assert(device != VK_NULL_HANDLE);
	m_device = device;

	// 1. Shader Stage
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	for (const auto& s : desc.shaders)
	{
		VkPipelineShaderStageCreateInfo stageInfo{};
		stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stageInfo.stage = s.stage;
		stageInfo.module = s.module;
		stageInfo.pName = s.entry;
		shaderStages.push_back(stageInfo);
	}

	// 2. Vertex Input
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(desc.vertexInput.bindings.size());
	vertexInputInfo.pVertexBindingDescriptions = desc.vertexInput.bindings.data();
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(desc.vertexInput.attributes.size());
	vertexInputInfo.pVertexAttributeDescriptions = desc.vertexInput.attributes.data();

	// 3. Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = desc.inputAssembly.topology;
	inputAssembly.primitiveRestartEnable = desc.inputAssembly.primitiveRestart ? VK_TRUE : VK_FALSE;

	// 4. Viewport / Scissor
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &desc.viewport.viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &desc.viewport.scissor;

	// 5. Rasterization
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = desc.raster.polygonMode;
	rasterizer.cullMode = desc.raster.cullMode;
	rasterizer.frontFace = desc.raster.frontFace;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.lineWidth = desc.raster.lineWidth;

	// 6. Multisample (多重采样)
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// 7. Depth Stencil
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = desc.depth.enableDepthTest ? VK_TRUE : VK_FALSE;
	depthStencil.depthWriteEnable = desc.depth.enableDepthWrite ? VK_TRUE : VK_FALSE;
	depthStencil.depthCompareOp = desc.depth.depthCompareOp;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	// 8. Color Blend
	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
	for (const auto& att : desc.blend.attachments)
	{
		VkPipelineColorBlendAttachmentState blend{};
		blend.blendEnable = att.enableBlend ? VK_TRUE : VK_FALSE;
		blend.srcColorBlendFactor = att.srcColorBlendFactor;
		blend.dstColorBlendFactor = att.dstColorBlendFactor;
		blend.colorBlendOp = att.colorBlendOp;
		blend.srcAlphaBlendFactor = att.srcAlphaBlendFactor;
		blend.dstAlphaBlendFactor = att.dstAlphaBlendFactor;
		blend.alphaBlendOp = att.alphaBlendOp;
		blend.colorWriteMask = att.colorWriteMask;
		colorBlendAttachments.push_back(blend);
	}

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
	colorBlending.pAttachments = colorBlendAttachments.data();

	// 9. Dynamic State
	std::vector<VkDynamicState> dynamicStates;
	if (desc.viewport.dynamic)
	{
		dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
	}

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	// 10. Pipeline Create Info
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = dynamicStates.empty() ? nullptr : &dynamicState;
	pipelineInfo.layout = desc.pipelineLayout;
	pipelineInfo.renderPass = desc.renderPass;
	pipelineInfo.subpass = desc.subpass;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline) != VK_SUCCESS)
		return false;

	return true;
}

void KPipeline::Destroy()
{
	if (m_pipeline != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(m_device, m_pipeline, nullptr);
		m_pipeline = VK_NULL_HANDLE;
	}
}
