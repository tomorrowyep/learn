#include "ksemaphore.h"
#include <utility>
#include <cassert>

KSemaphore::~KSemaphore()
{
	Destroy();
}

KSemaphore::KSemaphore(KSemaphore&& other) noexcept
{
	*this = std::move(other);
}

KSemaphore& KSemaphore::operator=(KSemaphore&& other) noexcept
{
	if (this != &other)
	{
		Destroy();

		m_device = other.m_device;
		m_semaphore = other.m_semaphore;

		other.m_device = VK_NULL_HANDLE;
		other.m_semaphore = VK_NULL_HANDLE;
	}
	return *this;
}

bool KSemaphore::Init(VkDevice device)
{
	assert(device != VK_NULL_HANDLE);

	Destroy();

	m_device = device;

	VkSemaphoreCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(m_device, &info, nullptr, &m_semaphore) != VK_SUCCESS)
	{
		m_semaphore = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

void KSemaphore::Destroy()
{
	if (m_semaphore != VK_NULL_HANDLE)
	{
		vkDestroySemaphore(m_device, m_semaphore, nullptr);
		m_semaphore = VK_NULL_HANDLE;
	}

	m_device = VK_NULL_HANDLE;
}
