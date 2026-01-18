#ifndef __RENDERPASSDESC_H__
#define __RENDERPASSDESC_H__

#include <vulkan.h>
#include <optional>

struct KAttachmentDesc
{
	VkFormat            format;
	VkSampleCountFlagBits samples;

	VkAttachmentLoadOp  loadOp;
	VkAttachmentStoreOp storeOp;

	VkAttachmentLoadOp  stencilLoadOp;
	VkAttachmentStoreOp stencilStoreOp;

	VkImageLayout       initialLayout;
	VkImageLayout       finalLayout;
};

struct KSubpassAttachmentRef
{
	uint32_t        attachmentIndex;
	VkImageLayout   layout;
};

struct KSubpassDesc
{
	VkPipelineBindPoint pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	std::vector<KSubpassAttachmentRef> colorAttachments;
	std::vector<KSubpassAttachmentRef> resolveAttachments;

	std::optional<KSubpassAttachmentRef> depthStencilAttachment;
};

struct KSubpassDependencyDesc
{
	uint32_t srcSubpass;
	uint32_t dstSubpass;

	VkPipelineStageFlags srcStageMask;
	VkPipelineStageFlags dstStageMask;

	VkAccessFlags srcAccessMask;
	VkAccessFlags dstAccessMask;

	VkDependencyFlags dependencyFlags = 0;
};

struct KRenderPassCreateDesc
{
	std::vector<KAttachmentDesc>        attachments;
	std::vector<KSubpassDesc>           subpasses;
	std::vector<KSubpassDependencyDesc> dependencies;
};

#endif // !__RENDERPASSDESC_H__
