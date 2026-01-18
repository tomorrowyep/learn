#include "ksampler.h"
#include <cassert>

KSampler::~KSampler()
{
	Destroy();
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
