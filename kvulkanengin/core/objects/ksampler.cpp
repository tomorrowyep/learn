#include "ksampler.h"
#include <cassert>
#include <utility>

KSampler::~KSampler()
{
	Destroy();
}

KSampler::KSampler(KSampler&& other) noexcept
{
	*this = std::move(other);
}

KSampler& KSampler::operator=(KSampler&& other) noexcept
{
	if (this != &other)
	{
		Destroy();

		m_device = other.m_device;
		m_sampler = other.m_sampler;

		other.m_device = VK_NULL_HANDLE;
		other.m_sampler = VK_NULL_HANDLE;
	}
	return *this;
}

bool KSampler::Init(
	VkDevice device,
	const VkSamplerCreateInfo& createInfo)
{
	assert(device != VK_NULL_HANDLE);
	Destroy(); // 支持重复 Init，重建时非常有用

	m_device = device;
	if (vkCreateSampler(m_device, &createInfo, nullptr, &m_sampler) != VK_SUCCESS)
	{
		m_sampler = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

void KSampler::Destroy()
{
	if (m_sampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(m_device, m_sampler, nullptr);
		m_sampler = VK_NULL_HANDLE;
	}

	m_device = VK_NULL_HANDLE;
}
