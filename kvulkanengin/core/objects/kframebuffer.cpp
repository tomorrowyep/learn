#include "kframebuffer.h"
#include <cassert>

KFramebuffer::~KFramebuffer()
{
	Destroy();
}

bool KFramebuffer::Init(
	VkDevice device,
	VkRenderPass renderPass,
	const std::vector<VkImageView>& attachments,
	uint32_t width,
	uint32_t height
)
{
	assert(device != VK_NULL_HANDLE);
	assert(renderPass != VK_NULL_HANDLE);
	assert(!attachments.empty());
	assert(width > 0 && height > 0);

	// 支持重复 Init，重建时非常有用
	Destroy();

	m_device = device;

	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass;           // 关联的 RenderPass
	framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	framebufferInfo.pAttachments = attachments.data(); // 关联的 ImageView
	framebufferInfo.width = width;                     // 宽度
	framebufferInfo.height = height;                   // 高度
	framebufferInfo.layers = 1;                        // 图像层数，通常为 1

	if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_framebuffer) != VK_SUCCESS)
	{
		m_framebuffer = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

void KFramebuffer::Destroy()
{
	// 只有在设备和 framebuffer 都有效时才销毁
	if (m_framebuffer != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE)
	{
		vkDestroyFramebuffer(m_device, m_framebuffer, nullptr);
		m_framebuffer = VK_NULL_HANDLE;
	}

	m_device = VK_NULL_HANDLE;
}

KFramebuffer::KFramebuffer(KFramebuffer&& other) noexcept
{
	*this = std::move(other);
}

KFramebuffer& KFramebuffer::operator=(KFramebuffer&& other) noexcept
{
	if (this != &other)
	{
		Destroy();

		m_device = other.m_device;
		m_framebuffer = other.m_framebuffer;

		other.m_device = VK_NULL_HANDLE;
		other.m_framebuffer = VK_NULL_HANDLE;
	}

	return *this;
}
