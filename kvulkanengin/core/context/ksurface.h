#ifndef __KSURFACE_H__
#define __KSURFACE_H__

#include <vulkan.h>

class KSurface
{
public:
	KSurface() = default;
	KSurface(VkInstance instance) : m_instance(instance) {};
	~KSurface() { Destroy(); }

	void Attach(VkSurfaceKHR surface) { m_surface = surface; }
	void SetInstance(VkInstance instance) { m_instance = instance; }

	void Destroy()
	{
		if (m_surface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
			m_surface = VK_NULL_HANDLE;
		}
	}

	VkSurfaceKHR GetHandle() const { return m_surface; }

private:
	VkInstance   m_instance{ VK_NULL_HANDLE };
	VkSurfaceKHR m_surface{ VK_NULL_HANDLE };
};

#endif // !__KSURFACE_H__
