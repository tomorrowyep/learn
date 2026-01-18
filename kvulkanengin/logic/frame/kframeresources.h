#ifndef __KKFrameResources_H__
#define __KKFrameResources_H__

#include "core/context/kdevice.h"
#include "core/commands/kcommandpool.h"
#include "core/commands/kcommandbuffer.h"
#include "core/sync/kfence.h"
#include "core/sync/ksemaphore.h"

class KFrameResources
{
public:
	KFrameResources() = default;
	~KFrameResources();

	KFrameResources(const KFrameResources&) = delete;
	KFrameResources& operator=(const KFrameResources&) = delete;

	KFrameResources(KFrameResources&& other) noexcept;
	KFrameResources& operator=(KFrameResources&& other) noexcept;

	bool Init(KDevice& device);
	void Destroy();

	KCommandBuffer& GetCommandBuffer() { return m_cmdBuffer; }
	KFence& GetInFlightFence() { return m_inFlightFence; }
	KSemaphore& GetImageAvailableSemaphore() { return m_imageAvailable; }
	KSemaphore& GetRenderFinishedSemaphore() { return m_renderFinished; }

private:
	KDevice* m_device = nullptr;
	KCommandPool   m_cmdPool;
	KCommandBuffer m_cmdBuffer;

	KFence     m_inFlightFence;
	KSemaphore m_imageAvailable;
	KSemaphore m_renderFinished;
};

#endif // !__KKFrameResources_H__
