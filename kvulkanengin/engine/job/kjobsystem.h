#ifndef __KJOBSYSTEM_H__
#define __KJOBSYSTEM_H__

#include "kthreadpool.h"
#include "core/context/kcorecontext.h"
#include "core/commands/kcommandpool.h"
#include "core/commands/kcommandbuffer.h"
#include <memory>
#include <vector>

namespace kEngine
{

static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
static constexpr uint32_t COMMAND_BUFFERS_PER_THREAD = MAX_FRAMES_IN_FLIGHT;

struct KThreadRenderContext
{
	KCommandPool                commandPool;
	std::vector<KCommandBuffer> commandBuffers;
};

class KJobSystem
{
public:
	KJobSystem() = default;
	~KJobSystem() = default;

	KJobSystem(const KJobSystem&) = delete;
	KJobSystem& operator=(const KJobSystem&) = delete;

	void Init(KCoreContext* ctx, uint32_t threadCount = 0);
	void Destroy();

	KThreadRenderContext& GetThreadContext(uint32_t threadIndex);
	KCommandBuffer& GetSecondaryBuffer(uint32_t threadIndex, uint32_t frameIndex);

	template<typename F>
	void ParallelFor(uint32_t count, F&& task)
	{
		if (!m_threadPool || count == 0)
			return;

		std::vector<std::future<void>> futures;
		futures.reserve(count);

		for (uint32_t i = 0; i < count; ++i)
		{
			futures.push_back(m_threadPool->Submit([&task, i]()
			{
				task(i);
			}));
		}

		for (auto& f : futures)
		{
			f.get();
		}
	}

	void WaitIdle();

	uint32_t GetThreadCount() const;
	bool IsInitialized() const { return m_initialized; }

private:
	void CreateThreadContexts();
	void DestroyThreadContexts();

private:
	KCoreContext*                         m_ctx = nullptr;
	std::unique_ptr<KThreadPool>          m_threadPool;
	std::vector<KThreadRenderContext>     m_threadContexts;
	bool                                  m_initialized = false;
};

} // namespace kEngine

#endif // !__KJOBSYSTEM_H__
