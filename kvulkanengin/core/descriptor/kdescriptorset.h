#ifndef __KDESCRIPTORSET_H__
#define __KDESCRIPTORSET_H__

#include <vulkan.h>
#include <vector>

class KDescriptorSet
{
public:
	KDescriptorSet() = default;
	~KDescriptorSet();

	KDescriptorSet(const KDescriptorSet&) = delete;
	KDescriptorSet& operator=(const KDescriptorSet&) = delete;

	bool Allocate(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout);
	void Update(const std::vector<VkWriteDescriptorSet>& writes);

	VkDescriptorSet GetHandle() const { return m_set; }

private:
	VkDevice m_device{ VK_NULL_HANDLE };
	VkDescriptorSet m_set{ VK_NULL_HANDLE };
};

#endif // !__KDESCRIPTORSET_H__
