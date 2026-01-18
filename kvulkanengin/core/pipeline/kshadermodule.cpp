#include "kshadermodule.h"

#include <cassert>

KShaderModule::~KShaderModule()
{
	Destroy();
}

bool KShaderModule::Init(VkDevice device, const std::vector<uint32_t>& spirv)
{
	assert(device != VK_NULL_HANDLE);
	assert(!spirv.empty());

	// 支持重复 Init，重建时非常有用
	Destroy();

	m_device = device;

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = spirv.size() * sizeof(uint32_t);
	createInfo.pCode = spirv.data();

	VkResult res = vkCreateShaderModule(
		m_device,
		&createInfo,
		nullptr,
		&m_module
	);

	if (res != VK_SUCCESS)
	{
		m_module = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

void KShaderModule::Destroy()
{
	if (m_module != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(m_device, m_module, nullptr);
		m_module = VK_NULL_HANDLE;
	}

	m_device = VK_NULL_HANDLE;
}
