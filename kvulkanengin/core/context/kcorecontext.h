#ifndef __KINCONTEXT_H__
#define __KINCONTEXT_H__

#include <vulkan.h>

#include "kinstance.h"
#include "ksurface.h"
#include "kphysicaldevice.h"
#include "kdevice.h"

// 核心上下文管理类
class KCoreContext
{
public:
	KCoreContext() = default;
	~KCoreContext();

	// 禁止拷贝
	KCoreContext(const KCoreContext&) = delete;
	KCoreContext& operator=(const KCoreContext&) = delete;

	bool CreateInstance(const KInstanceDesc& desc);
	void Attach(VkSurfaceKHR surface);
	bool CreateDevice(const KDeviceCreateDesc& desc);

	void Destroy();

	// 全局访问接口
	KInstance& GetInstance() { return m_instance; }
	KSurface& GetSurface() { return m_surface; }
	KPhysicalDevice& GetPhysicalDevice() { return m_physicalDevice; }
	KDevice& GetDevice() { return m_device; }

private:
	KInstance        m_instance;
	KSurface         m_surface;
	KPhysicalDevice m_physicalDevice;
	KDevice          m_device;
};

#endif // !__KINCONTEXT_H__
