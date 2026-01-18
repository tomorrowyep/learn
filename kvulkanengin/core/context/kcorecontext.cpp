#include "kcorecontext.h"

KCoreContext::~KCoreContext()
{
	Destroy();
}

bool KCoreContext::CreateInstance(const KInstanceDesc& desc)
{
	if (!m_instance.Create(desc))
		return false;

	return true;
}

void KCoreContext::Attach(VkSurfaceKHR surface)
{
	m_surface.SetInstance(m_instance.GetHandle());
	m_surface.Attach(surface);
}

bool KCoreContext::CreateDevice(const KDeviceCreateDesc& desc)
{
	if (!m_physicalDevice.Pick(m_instance.GetHandle(),
		m_surface.GetHandle(),
		desc))
		return false;

	if (!m_device.Create(m_physicalDevice, desc))
		return false;

	return true;
}

void KCoreContext::Destroy()
{
	m_device.Destroy();
	m_surface.Destroy();
	m_instance.Destroy();
}
