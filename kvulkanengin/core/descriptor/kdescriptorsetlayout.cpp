#include "kdescriptorsetlayout.h"
#include <cassert>

KDescriptorSetLayout::~KDescriptorSetLayout()
{
	Destroy();
}

bool KDescriptorSetLayout::Init(
	VkDevice device,
	const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
	assert(device != VK_NULL_HANDLE);
	assert(!bindings.empty());

	m_device = device;

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();

	VkResult ret = vkCreateDescriptorSetLayout(
		m_device,
		&createInfo,
		nullptr,
		&m_layout
	);

	if (ret != VK_SUCCESS)
	{
		m_layout = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

void KDescriptorSetLayout::Destroy()
{
	if (m_layout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(m_device, m_layout, nullptr);
		m_layout = VK_NULL_HANDLE;
	}

	m_device = VK_NULL_HANDLE;
}
