#ifndef __INSTANCE_DESC_H__
#define __INSTANCE_DESC_H__

#include <vulkan.h>
#include <vector>

struct KInstanceDesc
{
	const char* applicationName = "App";
	uint32_t    applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	const char* engineName = "NO Engine";
	uint32_t    engineVersion = VK_MAKE_VERSION(1, 0, 0);
	uint32_t    apiVersion = VK_API_VERSION_1_0;
	bool        benableValidation = false;
	std::vector<const char*> extensions;
};

#endif // !__INSTANCE_DESC_H__
