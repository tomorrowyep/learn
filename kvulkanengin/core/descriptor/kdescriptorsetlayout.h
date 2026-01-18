#ifndef __KDESCRIPTORSETLAYOUT_H__
#define __KDESCRIPTORSETLAYOUT_H__

#include <vulkan.h>
#include <vector>

class KDescriptorSetLayout
{
public:
	KDescriptorSetLayout() = default;
	~KDescriptorSetLayout();

	KDescriptorSetLayout(const KDescriptorSetLayout&) = delete;
	KDescriptorSetLayout& operator=(const KDescriptorSetLayout&) = delete;

	bool Init(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& bindings);
	void Destroy();

	VkDescriptorSetLayout GetHandle() const { return m_layout; }

private:
	VkDevice m_device{ VK_NULL_HANDLE };
	VkDescriptorSetLayout m_layout{ VK_NULL_HANDLE };
};

#endif // !__KDESCRIPTORSETLAYOUT_H__
