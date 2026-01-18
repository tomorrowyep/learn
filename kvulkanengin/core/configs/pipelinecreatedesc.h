#ifndef __PIPELINECREATEDESC_H__
#define __PIPELINECREATEDESC_H__

#include <vulkan.h>
#include <vector>

struct PipelineShaderStageDesc
{
	VkShaderStageFlagBits stage;
	VkShaderModule        module;
	const char* entry = "main";
};

struct PipelineVertexInputDesc
{
	std::vector<VkVertexInputBindingDescription>   bindings;
	std::vector<VkVertexInputAttributeDescription> attributes;
};

struct PipelineInputAssemblyDesc
{
	VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	bool primitiveRestart = false;
};

struct PipelineViewportDesc
{
	bool dynamic = false;
	VkViewport viewport{};
	VkRect2D   scissor{};
};

struct PipelineRasterDesc
{
	VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
	VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
	VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	float lineWidth = 1.0f;
};

struct PipelineDepthDesc
{
	bool enableDepthTest = false;
	bool enableDepthWrite = false;
	VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
};

struct PipelineBlendAttachmentDesc
{
	bool enableBlend = false;
	VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	VkBlendOp colorBlendOp = VK_BLEND_OP_ADD;
	VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD;
	VkColorComponentFlags colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
};

struct PipelineBlendDesc
{
	std::vector<PipelineBlendAttachmentDesc> attachments;
};

struct KPipelineCreateDesc
{
	VkPipelineLayout pipelineLayout;
	VkRenderPass     renderPass;
	uint32_t         subpass = 0;

	std::vector<PipelineShaderStageDesc> shaders;

	PipelineVertexInputDesc   vertexInput;
	PipelineInputAssemblyDesc inputAssembly;
	PipelineViewportDesc      viewport;
	PipelineRasterDesc        raster;
	PipelineDepthDesc         depth;
	PipelineBlendDesc         blend;
};

#endif // __PIPELINECREATEDESC_H__
