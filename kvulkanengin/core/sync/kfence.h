#ifndef __KFENCE_H__
#define __KFENCE_H__

#include <vulkan.h>

class KFence
{
public:
	KFence() = default;
	~KFence();

	KFence(const KFence&) = delete;
	KFence& operator=(const KFence&) = delete;

	// 支持移动语义，用于 FrameResource
	KFence(KFence&& other) noexcept;
	KFence& operator=(KFence&& other) noexcept;

	bool Init(
		VkDevice device,
		bool signaled = false   // 初始状态是否为 signaled
	);

	void Destroy();

	// 等待 fence
	bool Wait(uint64_t timeout = UINT64_MAX) const;

	// 重置 fence，将其设置为未信号状态
	void Reset() const;

	VkFence GetHandle() const { return m_fence; }

private:
	VkDevice m_device{ VK_NULL_HANDLE };
	VkFence  m_fence{ VK_NULL_HANDLE };
};

#endif // __KFENCE_H__
