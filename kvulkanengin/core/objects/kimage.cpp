#include "kimage.h"

#include <cassert>
#include <vector>

// 查找内存类型（可以移到 Utils / MemoryHelpers 中）
static uint32_t FindMemoryType(
	VkPhysicalDevice physicalDevice,
	uint32_t typeFilter,
	VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties{};
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
	{
		if ((typeFilter & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	assert(false && "Failed to find suitable memory type for image");
	return 0;
}

KImage::~KImage()
{
	Destroy();
}

KImage::KImage(KImage&& other) noexcept
{
	*this = std::move(other);
}

KImage& KImage::operator=(KImage&& other) noexcept
{
	if (this != &other)
	{
		Destroy();

		m_device = other.m_device;
		m_image = other.m_image;
		m_memory = other.m_memory;
		m_format = other.m_format;

		other.m_device = VK_NULL_HANDLE;
		other.m_image = VK_NULL_HANDLE;
		other.m_memory = VK_NULL_HANDLE;
		other.m_format = VK_FORMAT_UNDEFINED;
	}
	return *this;
}

bool KImage::Init(
	VkDevice device,
	VkPhysicalDevice physicalDevice,
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkImageUsageFlags usage,
	VkImageTiling tiling,
	VkMemoryPropertyFlags memoryProperties)
{
	assert(device != VK_NULL_HANDLE);
	assert(physicalDevice != VK_NULL_HANDLE);
	assert(width > 0 && height > 0);

	m_device = device;
	m_format = format;

	// 创建 VkImage
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;        // Mip 级别
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(device, &imageInfo, nullptr, &m_image) != VK_SUCCESS)
		return false;

	// 查询 Image 内存需求
	VkMemoryRequirements memRequirements{};
	vkGetImageMemoryRequirements(device, m_image, &memRequirements);

	// 3. 分配内存
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex =
		FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, memoryProperties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS)
	{
		Destroy();
		return false;
	}

	// 绑定 Image 和 Memory
	vkBindImageMemory(device, m_image, m_memory, 0);

	return true;
}

void KImage::Destroy()
{
	if (m_image != VK_NULL_HANDLE)
	{
		vkDestroyImage(m_device, m_image, nullptr);
		m_image = VK_NULL_HANDLE;
	}

	if (m_memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(m_device, m_memory, nullptr);
		m_memory = VK_NULL_HANDLE;
	}

	m_device = VK_NULL_HANDLE;
	m_format = VK_FORMAT_UNDEFINED;
}
