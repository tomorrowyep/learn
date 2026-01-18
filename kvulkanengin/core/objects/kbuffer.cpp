#include "kbuffer.h"
#include <cassert>
#include <utility>

static uint32_t _findMemoryType(
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

	assert(false && "Failed to find suitable memory type");
	return 0;
}

KBuffer::~KBuffer()
{
	Destroy();
}

KBuffer::KBuffer(KBuffer&& other) noexcept
{
	*this = std::move(other);
}

KBuffer& KBuffer::operator=(KBuffer&& other) noexcept
{
	if (this != &other)
	{
		Destroy();

		m_device = other.m_device;
		m_buffer = other.m_buffer;
		m_memory = other.m_memory;
		m_size = other.m_size;

		other.m_device = VK_NULL_HANDLE;
		other.m_buffer = VK_NULL_HANDLE;
		other.m_memory = VK_NULL_HANDLE;
		other.m_size = 0;
	}

	return *this;
}

bool KBuffer::Init(
	VkDevice device,
	VkPhysicalDevice physicalDevice,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags memoryProperties)
{
	assert(device != VK_NULL_HANDLE);
	assert(size > 0);

	m_device = device;
	m_size = size;

	// 1. 创建 VkBuffer
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS)
		return false;

	// 2. 查询内存需求
	VkMemoryRequirements memRequirements{};
	vkGetBufferMemoryRequirements(device, m_buffer, &memRequirements);

	// 3. 分配内存
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex =_findMemoryType(physicalDevice, memRequirements.memoryTypeBits, memoryProperties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS)
	{
		Destroy();
		return false;
	}

	// 4. 绑定内存
	vkBindBufferMemory(device, m_buffer, m_memory, 0);

	return true;
}

void KBuffer::Destroy()
{
	if (m_buffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(m_device, m_buffer, nullptr);
		m_buffer = VK_NULL_HANDLE;
	}

	if (m_memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(m_device, m_memory, nullptr);
		m_memory = VK_NULL_HANDLE;
	}

	m_device = VK_NULL_HANDLE;
	m_size = 0;
}

void* KBuffer::Map()
{
	assert(m_memory != VK_NULL_HANDLE);

	void* data = nullptr;
	VkResult result = vkMapMemory(
		m_device,
		m_memory,
		0,
		m_size,
		0,
		&data
	);

	assert(result == VK_SUCCESS);
	return data;
}

void KBuffer::Unmap()
{
	assert(m_memory != VK_NULL_HANDLE);
	vkUnmapMemory(m_device, m_memory);
}
