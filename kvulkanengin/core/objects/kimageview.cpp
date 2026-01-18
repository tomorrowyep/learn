#include "kimageview.h"

#include <cassert>
#include <utility>

KImageView::~KImageView()
{
	Destroy();
}

KImageView::KImageView(KImageView&& other) noexcept
{
	*this = std::move(other);
}

KImageView& KImageView::operator=(KImageView&& other) noexcept
{
	if (this != &other)
	{
		Destroy();

		m_device = other.m_device;
		m_view = other.m_view;

		other.m_device = VK_NULL_HANDLE;
		other.m_view = VK_NULL_HANDLE;
	}
	return *this;
}

bool KImageView::Init(
	VkDevice device,
	VkImage image,
	VkFormat format,
	VkImageAspectFlags aspect)
{
	assert(device != VK_NULL_HANDLE);
	assert(image != VK_NULL_HANDLE);

	m_device = device;

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;

	// subresource 范围
	viewInfo.subresourceRange.aspectMask = aspect;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(device, &viewInfo, nullptr, &m_view) != VK_SUCCESS)
		return false;

	return true;
}

void KImageView::Destroy()
{
	// 只有在设备有效时才销毁 image view
	if (m_view != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE)
	{
		vkDestroyImageView(m_device, m_view, nullptr);
		m_view = VK_NULL_HANDLE;
	}

	m_device = VK_NULL_HANDLE;
}
