#include "kdescriptorSet.h"
#include <cassert>

KDescriptorSet::~KDescriptorSet()
{
	// 注意：这里不需要 free
	// DescriptorSet 的生命周期由 Pool 管理
}

bool KDescriptorSet::Allocate(
	VkDevice device,
	VkDescriptorPool pool,
	VkDescriptorSetLayout layout)
{
	assert(device != VK_NULL_HANDLE);
	assert(pool != VK_NULL_HANDLE);
	assert(layout != VK_NULL_HANDLE);

	m_device = device;

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	VkResult ret = vkAllocateDescriptorSets(
		m_device,
		&allocInfo,
		&m_set
	);

	if (ret != VK_SUCCESS)
	{
		m_set = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

void KDescriptorSet::Update(const std::vector<VkWriteDescriptorSet>& writes)
{
	assert(m_set != VK_NULL_HANDLE);
	assert(!writes.empty());

	// Vulkan 要求
	// VkWriteDescriptorSet::dstSet 必须指向目标 DescriptorSet
	// 这里统一设置所有 writes 的 dstSet
	std::vector<VkWriteDescriptorSet> fixedWrites = writes;
	for (auto& w : fixedWrites)
	{
		w.dstSet = m_set;
	}

	vkUpdateDescriptorSets(
		m_device,
		static_cast<uint32_t>(fixedWrites.size()),
		fixedWrites.data(),
		0,
		nullptr
	);
}
