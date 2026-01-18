#ifndef __KCOMMANDPOOL_H__
#define __KCOMMANDPOOL_H__

#include <vulkan.h>

class KCommandPool
{
public:
	KCommandPool() = default;
	~KCommandPool();

	KCommandPool(const KCommandPool&) = delete;
	KCommandPool& operator=(const KCommandPool&) = delete;

	KCommandPool(KCommandPool&& other) noexcept;
	KCommandPool& operator=(KCommandPool&& other) noexcept;

	bool Init(VkDevice device,
		uint32_t queueFamilyIndex,
		VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	void Destroy();

	VkCommandPool GetHandle() const { return m_pool; }

private:
	VkDevice       m_device{ VK_NULL_HANDLE };
	VkCommandPool  m_pool{ VK_NULL_HANDLE };
};

#endif // !__KCOMMANDPOOL_H__
