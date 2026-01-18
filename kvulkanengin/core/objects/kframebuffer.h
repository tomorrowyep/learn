#ifndef __KFRAMEBUFFER_H__
#define __KFRAMEBUFFER_H__

#include <vulkan.h>
#include <vector>

class KFramebuffer
{
public:
	KFramebuffer() = default;
	~KFramebuffer();

	KFramebuffer(const KFramebuffer&) = delete;
	KFramebuffer& operator=(const KFramebuffer&) = delete;

	KFramebuffer(KFramebuffer&& other) noexcept;
	KFramebuffer& operator=(KFramebuffer&& other) noexcept;

	bool Init(
		VkDevice device,
		VkRenderPass renderPass,
		const std::vector<VkImageView>& attachments,
		uint32_t width,
		uint32_t height
	);

	void Destroy();

	VkFramebuffer GetHandle() const { return m_framebuffer; }

private:
	VkDevice       m_device = VK_NULL_HANDLE;
	VkFramebuffer  m_framebuffer = VK_NULL_HANDLE;
};

#endif // !__KFRAMEBUFFER_H__
