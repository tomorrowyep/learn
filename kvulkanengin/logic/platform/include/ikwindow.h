#ifndef __IKWINDOW_H__
#define __IKWINDOW_H__

#include <vulkan.h>

class IKWindow
{
public:
	virtual ~IKWindow() = default;

	virtual uint32_t GetWidth() const = 0;
	virtual uint32_t GetHeight() const = 0;

	virtual bool ShouldClose() const = 0;
	virtual void PollEvents() = 0;

	virtual VkSurfaceKHR CreateSurface(VkInstance instance) = 0;
};

#endif // !__IKWINDOW_H__
