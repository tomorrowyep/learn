#ifndef __KBUFFER_H__
#define __KBUFFER_H__

#include <vulkan.h>

class KBuffer
{
public:
	KBuffer() = default;
	~KBuffer();

	KBuffer(const KBuffer&) = delete;
	KBuffer& operator=(const KBuffer&) = delete;
	KBuffer(KBuffer&&) noexcept;
	KBuffer& operator=(KBuffer&&) noexcept;

	bool Init(
		VkDevice device,
		VkPhysicalDevice physicalDevice,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags memoryProperties
	);

	void Destroy();

	VkBuffer GetHandle() const { return m_buffer; }
	VkDeviceSize GetSize() const { return m_size; }

	void* Map();
	void  Unmap();

private:
	VkDevice        m_device = VK_NULL_HANDLE;
	VkBuffer        m_buffer = VK_NULL_HANDLE;
	VkDeviceMemory  m_memory = VK_NULL_HANDLE;
	VkDeviceSize    m_size = 0;
};
#endif // !__KBUFFER_H__
