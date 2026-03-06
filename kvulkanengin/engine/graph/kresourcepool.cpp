#include "kresourcepool.h"
#include <iostream>

namespace kEngine
{

void KResourcePool::Init(KCoreContext* ctx)
{
	m_ctx = ctx;
}

void KResourcePool::Destroy()
{
	for (auto& tex : m_textures)
	{
		DestroyResource(tex.resource);
	}
	m_textures.clear();

	for (auto& buf : m_buffers)
	{
		DestroyResource(buf.resource);
	}
	m_buffers.clear();

	m_ctx = nullptr;
}

void KResourcePool::ResetFrame()
{
	for (auto& tex : m_textures)
	{
		tex.inUse = false;
	}
	for (auto& buf : m_buffers)
	{
		buf.inUse = false;
	}
}

PhysicalResource KResourcePool::AcquireTexture(const TextureDesc& desc)
{
	for (auto& tex : m_textures)
	{
		if (!tex.inUse &&
		    tex.desc.width == desc.width &&
		    tex.desc.height == desc.height &&
		    tex.desc.format == desc.format &&
		    tex.desc.usage == desc.usage)
		{
			tex.inUse = true;
			return tex.resource;
		}
	}

	PooledTexture pooled;
	pooled.desc = desc;
	pooled.resource = CreateTexture(desc);
	pooled.inUse = true;
	m_textures.push_back(pooled);

	return pooled.resource;
}

void KResourcePool::ReleaseTexture(PhysicalResource& resource)
{
	for (auto& tex : m_textures)
	{
		if (tex.resource.image == resource.image)
		{
			tex.inUse = false;
			return;
		}
	}
}

PhysicalResource KResourcePool::AcquireBuffer(const BufferDesc& desc)
{
	for (auto& buf : m_buffers)
	{
		if (!buf.inUse &&
		    buf.desc.size >= desc.size &&
		    buf.desc.usage == desc.usage)
		{
			buf.inUse = true;
			return buf.resource;
		}
	}

	PooledBuffer pooled;
	pooled.desc = desc;
	pooled.resource = CreateBuffer(desc);
	pooled.inUse = true;
	m_buffers.push_back(pooled);

	return pooled.resource;
}

void KResourcePool::ReleaseBuffer(PhysicalResource& resource)
{
	for (auto& buf : m_buffers)
	{
		if (buf.resource.buffer == resource.buffer)
		{
			buf.inUse = false;
			return;
		}
	}
}

PhysicalResource KResourcePool::CreateTexture(const TextureDesc& desc)
{
	PhysicalResource res;

	if (!m_ctx)
		return res;

	VkDevice device = m_ctx->GetDevice().GetHandle();
	VkPhysicalDevice physDevice = m_ctx->GetPhysicalDevice().GetHandle();

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = desc.width;
	imageInfo.extent.height = desc.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = desc.format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = desc.usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = desc.samples;

	if (vkCreateImage(device, &imageInfo, nullptr, &res.image) != VK_SUCCESS)
	{
		std::cerr << "[KResourcePool] Failed to create image" << std::endl;
		return res;
	}

	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(device, res.image, &memReq);

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(physDevice, &memProps);

	uint32_t memTypeIndex = 0;
	for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
	{
		if ((memReq.memoryTypeBits & (1 << i)) &&
		    (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
		{
			memTypeIndex = i;
			break;
		}
	}

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = memTypeIndex;

	if (vkAllocateMemory(device, &allocInfo, nullptr, &res.memory) != VK_SUCCESS)
	{
		vkDestroyImage(device, res.image, nullptr);
		res.image = VK_NULL_HANDLE;
		std::cerr << "[KResourcePool] Failed to allocate memory" << std::endl;
		return res;
	}

	vkBindImageMemory(device, res.image, res.memory, 0);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = res.image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = desc.format;

	bool isDepth = (desc.format == VK_FORMAT_D32_SFLOAT ||
	                desc.format == VK_FORMAT_D24_UNORM_S8_UINT ||
	                desc.format == VK_FORMAT_D16_UNORM);

	viewInfo.subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(device, &viewInfo, nullptr, &res.imageView) != VK_SUCCESS)
	{
		vkFreeMemory(device, res.memory, nullptr);
		vkDestroyImage(device, res.image, nullptr);
		res.image = VK_NULL_HANDLE;
		res.memory = VK_NULL_HANDLE;
		std::cerr << "[KResourcePool] Failed to create image view" << std::endl;
		return res;
	}

	return res;
}

PhysicalResource KResourcePool::CreateBuffer(const BufferDesc& desc)
{
	PhysicalResource res;

	if (!m_ctx)
		return res;

	VkDevice device = m_ctx->GetDevice().GetHandle();
	VkPhysicalDevice physDevice = m_ctx->GetPhysicalDevice().GetHandle();

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = desc.size;
	bufferInfo.usage = desc.usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &res.buffer) != VK_SUCCESS)
	{
		std::cerr << "[KResourcePool] Failed to create buffer" << std::endl;
		return res;
	}

	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(device, res.buffer, &memReq);

	VkPhysicalDeviceMemoryProperties memProps;
	vkGetPhysicalDeviceMemoryProperties(physDevice, &memProps);

	uint32_t memTypeIndex = 0;
	for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
	{
		if ((memReq.memoryTypeBits & (1 << i)) &&
		    (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
		{
			memTypeIndex = i;
			break;
		}
	}

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = memTypeIndex;

	if (vkAllocateMemory(device, &allocInfo, nullptr, &res.memory) != VK_SUCCESS)
	{
		vkDestroyBuffer(device, res.buffer, nullptr);
		res.buffer = VK_NULL_HANDLE;
		std::cerr << "[KResourcePool] Failed to allocate buffer memory" << std::endl;
		return res;
	}

	vkBindBufferMemory(device, res.buffer, res.memory, 0);

	return res;
}

void KResourcePool::DestroyResource(PhysicalResource& resource)
{
	if (!m_ctx)
		return;

	VkDevice device = m_ctx->GetDevice().GetHandle();

	if (resource.imageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(device, resource.imageView, nullptr);
		resource.imageView = VK_NULL_HANDLE;
	}

	if (resource.image != VK_NULL_HANDLE)
	{
		vkDestroyImage(device, resource.image, nullptr);
		resource.image = VK_NULL_HANDLE;
	}

	if (resource.buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(device, resource.buffer, nullptr);
		resource.buffer = VK_NULL_HANDLE;
	}

	if (resource.memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(device, resource.memory, nullptr);
		resource.memory = VK_NULL_HANDLE;
	}
}

size_t KResourcePool::HashTextureDesc(const TextureDesc& desc) const
{
	size_t h = 0;
	h ^= std::hash<uint32_t>()(desc.width);
	h ^= std::hash<uint32_t>()(desc.height) << 1;
	h ^= std::hash<uint32_t>()(static_cast<uint32_t>(desc.format)) << 2;
	h ^= std::hash<uint32_t>()(desc.usage) << 3;
	return h;
}

size_t KResourcePool::HashBufferDesc(const BufferDesc& desc) const
{
	size_t h = 0;
	h ^= std::hash<VkDeviceSize>()(desc.size);
	h ^= std::hash<uint32_t>()(desc.usage) << 1;
	return h;
}

} // namespace kEngine
