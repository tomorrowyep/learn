#ifndef __KDESCRIPTORPOOL_H__
#define __KDESCRIPTORPOOL_H__

#include <vulkan.h>
#include <vector>

class KDescriptorPool
{
public:
	KDescriptorPool() = default;
	~KDescriptorPool();

	KDescriptorPool(const KDescriptorPool&) = delete;
	KDescriptorPool& operator=(const KDescriptorPool&) = delete;

	bool Init(VkDevice device, const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets);
	void Destroy();

	VkDescriptorPool GetHandle() const { return m_pool; }

private:
	VkDevice m_device{ VK_NULL_HANDLE };
	VkDescriptorPool m_pool{ VK_NULL_HANDLE };
};

#endif // !__KDESCRIPTORPOOL_H__
