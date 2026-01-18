#ifndef __KSAMPLER_H__
#define __KSAMPLER_H__

#include <vulkan.h>

class KSampler
{
public:
	KSampler() = default;
	~KSampler();

	KSampler(const KSampler&) = delete;
	KSampler& operator=(const KSampler&) = delete;

	bool Init(
		VkDevice device,
		const VkSamplerCreateInfo& createInfo
	);

	void Destroy();

	VkSampler GetHandle() const { return m_sampler; }

private:
	VkDevice  m_device = VK_NULL_HANDLE;
	VkSampler m_sampler = VK_NULL_HANDLE;
};
#endif // !__KSAMPLER_H__
