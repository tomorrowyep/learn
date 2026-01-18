#ifndef __KINSTANCE_H__
#define __KINSTANCE_H__

#include <vulkan.h>
#include <vector>

#include "instancedesc.h"

class KInstance
{
public:
	KInstance();
	~KInstance();

	KInstance(const KInstance&) = delete;
	KInstance& operator=(const KInstance&) = delete;

	bool Create(const KInstanceDesc& desc);
	void Destroy();

	VkInstance GetHandle() const { return m_instance; }
	uint32_t   GetApiVersion() const { return m_apiVersion; }
	bool       IsValidationEnabled() const { return m_enableValidation; }

private:
	bool _checkValidationLayerSupport() const;
	VkResult _setupDebugMessenger();

private:
	VkInstance m_instance{ VK_NULL_HANDLE };
	std::vector<const char*> m_validationLayers;
	uint32_t   m_apiVersion{ VK_API_VERSION_1_0 };
	bool       m_enableValidation{ false };
	VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
};

#endif // !__KINSTANCE_H__
