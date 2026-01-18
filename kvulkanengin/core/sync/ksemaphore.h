#ifndef __KSEMAPHORE_H__
#define __KSEMAPHORE_H__

#include <vulkan.h>

class KSemaphore
{
public:
	KSemaphore() = default;
	~KSemaphore();

	KSemaphore(const KSemaphore&) = delete;
	KSemaphore& operator=(const KSemaphore&) = delete;

	KSemaphore(KSemaphore&& other) noexcept;
	KSemaphore& operator=(KSemaphore&& other) noexcept;

	bool Init(VkDevice device);
	void Destroy();

	VkSemaphore* GetHandle() { return &m_semaphore; }

private:
	VkDevice    m_device{ VK_NULL_HANDLE };
	VkSemaphore m_semaphore{ VK_NULL_HANDLE };
};

#endif // __KSEMAPHORE_H__
