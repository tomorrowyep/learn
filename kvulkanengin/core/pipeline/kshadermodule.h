#ifndef __KSHADERMODULE_H__
#define __KSHADERMODULE_H__

#include <vulkan.h>
#include <vector>

class KShaderModule
{
public:
	KShaderModule() = default;
	~KShaderModule();

	KShaderModule(const KShaderModule&) = delete;
	KShaderModule& operator=(const KShaderModule&) = delete;

	bool Init(VkDevice device, const std::vector<uint32_t>& spirv);
	void Destroy();

	VkShaderModule GetHandle() const { return m_module; }

private:
	VkDevice        m_device{ VK_NULL_HANDLE };
	VkShaderModule  m_module{ VK_NULL_HANDLE };
};
#endif
