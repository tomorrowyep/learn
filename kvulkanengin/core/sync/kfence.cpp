#include "KFence.h"
#include <utility>
#include <cassert>

KFence::~KFence()
{
	Destroy();
}

KFence::KFence(KFence&& other) noexcept
{
	*this = std::move(other);
}

KFence& KFence::operator=(KFence&& other) noexcept
{
	if (this != &other)
	{
		Destroy();

		m_device = other.m_device;
		m_fence = other.m_fence;

		other.m_device = VK_NULL_HANDLE;
		other.m_fence = VK_NULL_HANDLE;
	}
	return *this;
}

bool KFence::Init(VkDevice device, bool signaled)
{
	assert(device != VK_NULL_HANDLE);

	Destroy();

	m_device = device;

	VkFenceCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	if (vkCreateFence(m_device, &info, nullptr, &m_fence) != VK_SUCCESS)
	{
		m_fence = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

void KFence::Destroy()
{
	if (m_fence != VK_NULL_HANDLE)
	{
		vkDestroyFence(m_device, m_fence, nullptr);
		m_fence = VK_NULL_HANDLE;
	}

	m_device = VK_NULL_HANDLE;
}

bool KFence::Wait(uint64_t timeout) const
{
	assert(m_fence != VK_NULL_HANDLE);
	return vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, timeout) == VK_SUCCESS;
}

void KFence::Reset() const
{
	assert(m_fence != VK_NULL_HANDLE);
	vkResetFences(m_device, 1, &m_fence);
}
