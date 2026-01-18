#ifndef __KRENDERPASS_H__
#define __KRENDERPASS_H__

#include <vulkan.h>
#include <vector>
#include <optional>

struct KRenderPassCreateDesc;

class KRenderPass
{
public:
	KRenderPass() = default;
	~KRenderPass();

	KRenderPass(const KRenderPass&) = delete;
	KRenderPass& operator=(const KRenderPass&) = delete;

	KRenderPass(KRenderPass&&) noexcept;
	KRenderPass& operator=(KRenderPass&&) noexcept;

	bool Init(
		VkDevice device,
		const KRenderPassCreateDesc& desc
	);

	void Destroy();

	VkRenderPass GetHandle() const { return m_renderPass; }

private:
	VkDevice     m_device = VK_NULL_HANDLE;
	VkRenderPass m_renderPass = VK_NULL_HANDLE;
};

#endif // !__KRENDERPASS_H__
