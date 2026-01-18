#include "kcommandpool.h"
#include <utility>
#include <cassert>

KCommandPool::~KCommandPool()
{
	Destroy();
}

bool KCommandPool::Init(VkDevice device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
{
	assert(device != VK_NULL_HANDLE);

	m_device = device;

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndex;
	poolInfo.flags = flags;

	if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_pool) != VK_SUCCESS)
		return false;

	return true;
}

void KCommandPool::Destroy()
{
	if (m_pool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(m_device, m_pool, nullptr);
		m_pool = VK_NULL_HANDLE;
	}
}

KCommandPool::KCommandPool(KCommandPool&& other) noexcept
{
	*this = std::move(other);
}

KCommandPool& KCommandPool::operator=(KCommandPool&& other) noexcept
{
	if (this != &other)
	{
		Destroy();

		m_device = other.m_device;
		m_pool = other.m_pool;

		other.m_device = VK_NULL_HANDLE;
		other.m_pool = VK_NULL_HANDLE;
	}

	return *this;
}
