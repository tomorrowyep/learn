#include "kinstance.h"
#include <iostream>

static VKAPI_ATTR VkBool32 debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,   // 消息严重程度：信息 / 警告 / 错误
	VkDebugUtilsMessageTypeFlagsEXT messageType,              // 消息类型：通用 / 校验 / 性能
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,// 回调的消息结构体
	void* pUserData)                                          // 用户自定义数据指针
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

KInstance::KInstance()
{
	m_validationLayers.push_back("VK_LAYER_KHRONOS_validation");
}

KInstance::~KInstance()
{
	Destroy();
}

bool KInstance::Create(const KInstanceDesc& desc)
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = desc.applicationName;
	appInfo.applicationVersion = desc.applicationVersion;
	appInfo.pEngineName = desc.engineName;
	appInfo.engineVersion = desc.engineVersion;
	appInfo.apiVersion = desc.apiVersion;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// 启用验证层
	if (desc.benableValidation && _checkValidationLayerSupport())
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();
	}

	// 启用实例扩展
	createInfo.enabledExtensionCount = static_cast<uint32_t>(desc.extensions.size());
	createInfo.ppEnabledExtensionNames = desc.extensions.data();

	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
		return false;

	if (desc.benableValidation)
		_setupDebugMessenger();

	return true;
}

void KInstance::Destroy()
{
	// 销毁调试消息
	if (m_debugMessenger != VK_NULL_HANDLE)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");

		if (func != nullptr)
			func(m_instance, m_debugMessenger, nullptr);
	}

	// 销毁 Vulkan 实例
	if (m_instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(m_instance, nullptr);
		m_instance = VK_NULL_HANDLE;
	}
}

bool KInstance::_checkValidationLayerSupport() const
{
	uint32_t count = 0;
	vkEnumerateInstanceLayerProperties(&count, nullptr);

	std::vector<VkLayerProperties> layers(count);
	vkEnumerateInstanceLayerProperties(&count, layers.data());

	// 检查每个需要的验证层是否在可用层列表中
	for (const auto& name : m_validationLayers)
	{
		bool found = false;
		for (const auto& layerProp : layers)
		{
			if (strcmp(layerProp.layerName, name) == 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
			return false;
	}

	return true;
}

VkResult KInstance::_setupDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;
	createInfo.pNext = nullptr;

	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)
		vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
	return func ? func(m_instance, &createInfo, nullptr, &m_debugMessenger)
		: VK_ERROR_EXTENSION_NOT_PRESENT;
}
