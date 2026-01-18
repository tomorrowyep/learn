#ifndef __KPIPELINELAYOUT_H__
#define __KPIPELINELAYOUT_H__

#include <vulkan.h>
#include <vector>

class KPipelineLayout
{
public:
	KPipelineLayout() = default;
	~KPipelineLayout();

	KPipelineLayout(const KPipelineLayout&) = delete;
	KPipelineLayout& operator=(const KPipelineLayout&) = delete;

	bool Init(
		VkDevice device,
		const std::vector<VkDescriptorSetLayout>& setLayouts,
		const std::vector<VkPushConstantRange>& pushConstants = {}
	);

	void Destroy();

	VkPipelineLayout GetHandle() const { return m_layout; }

private:
	VkDevice           m_device{ VK_NULL_HANDLE };
	VkPipelineLayout   m_layout{ VK_NULL_HANDLE };
};

#endif
