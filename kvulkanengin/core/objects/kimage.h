#ifndef __KIMAGE_H__
#define __KIMAGE_H__

#include <vulkan.h>

class KImage
{
public:
	KImage() = default;
	~KImage();

	KImage(const KImage&) = delete;
	KImage& operator=(const KImage&) = delete;
	KImage(KImage&&) noexcept;
	KImage& operator=(KImage&&) noexcept;

	bool Init(
		VkDevice device,
		VkPhysicalDevice physicalDevice,
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkImageUsageFlags usage,
		VkImageTiling tiling,
		VkMemoryPropertyFlags memoryProperties
	);

	void Destroy();

	VkImage  GetHandle() const { return m_image; }
	VkFormat GetFormat() const { return m_format; }

private:
	VkDevice        m_device = VK_NULL_HANDLE;
	VkImage         m_image = VK_NULL_HANDLE;
	VkDeviceMemory  m_memory = VK_NULL_HANDLE;
	VkFormat        m_format = VK_FORMAT_UNDEFINED;
};

#endif // !__KIMAGE_H__
