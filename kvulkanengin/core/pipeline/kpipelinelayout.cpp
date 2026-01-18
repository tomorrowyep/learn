#include "kpipelinelayout.h"

#include <cassert>

KPipelineLayout::~KPipelineLayout()
{
	Destroy();
}

bool KPipelineLayout::Init(
	VkDevice device,
	const std::vector<VkDescriptorSetLayout>& setLayouts,
	const std::vector<VkPushConstantRange>& pushConstants
)
{
	assert(device != VK_NULL_HANDLE);

	// 支持重复 Init，重建时非常有用
	Destroy();

	m_device = device;

	VkPipelineLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	// Descriptor Set Layout
	createInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
	createInfo.pSetLayouts = setLayouts.empty() ? nullptr : setLayouts.data();

	// Push Constants
	createInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
	createInfo.pPushConstantRanges = pushConstants.empty() ? nullptr : pushConstants.data();

	VkResult res = vkCreatePipelineLayout(
		m_device,
		&createInfo,
		nullptr,
		&m_layout
	);

	if (res != VK_SUCCESS)
	{
		m_layout = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

void KPipelineLayout::Destroy()
{
	if (m_layout != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(m_device, m_layout, nullptr);
		m_layout = VK_NULL_HANDLE;
	}

	m_device = VK_NULL_HANDLE;
}
