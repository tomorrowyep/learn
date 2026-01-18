#ifndef __KIMAGEVIEW_H__
#define __KIMAGEVIEW_H__

#include <vulkan.h>

class KImageView
{
public:
	KImageView() = default;
	~KImageView();

	KImageView(const KImageView&) = delete;
	KImageView& operator=(const KImageView&) = delete;
	KImageView(KImageView&&) noexcept;
	KImageView& operator=(KImageView&&) noexcept;

	bool Init(
		VkDevice device,
		VkImage image,
		VkFormat format,
		VkImageAspectFlags aspect
	);

	void Destroy();

	VkImageView GetHandle() const { return m_view; }

private:
	VkDevice     m_device = VK_NULL_HANDLE;
	VkImageView  m_view = VK_NULL_HANDLE;
};
#endif // !__KIMAGEVIEW_H__
