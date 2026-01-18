#include "kdescriptorpool.h"
#include <cassert>

KDescriptorPool::~KDescriptorPool()
{
	Destroy();
}

bool KDescriptorPool::Init(
	VkDevice device,
	const std::vector<VkDescriptorPoolSize>& poolSizes,
	uint32_t maxSets)
{
	assert(device != VK_NULL_HANDLE);
	assert(!poolSizes.empty());
	assert(maxSets > 0);

	m_device = device;

	VkDescriptorPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = maxSets;

	// 如果需要单独释放 vkFreeDescriptorSets
	// 可以设置此标志，允许单独释放 DescriptorSet
	// createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	VkResult ret = vkCreateDescriptorPool(
		m_device,
		&createInfo,
		nullptr,
		&m_pool
	);

	if (ret != VK_SUCCESS)
	{
		m_pool = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

void KDescriptorPool::Destroy()
{
	if (m_pool != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(m_device, m_pool, nullptr);
		m_pool = VK_NULL_HANDLE;
	}

	m_device = VK_NULL_HANDLE;
}
