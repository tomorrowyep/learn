#include "kjobsystem.h"
#include <iostream>

namespace kEngine
{

void KJobSystem::Init(KCoreContext* ctx, uint32_t threadCount)
{
	if (m_initialized)
		return;

	m_ctx = ctx;

	if (threadCount == 0)
	{
		threadCount = std::max(1u, std::thread::hardware_concurrency() - 1);
	}

	m_threadPool = std::make_unique<KThreadPool>(threadCount);
	CreateThreadContexts();

	m_initialized = true;
	std::cout << "[KJobSystem] Initialized with " << threadCount << " threads" << std::endl;
}

void KJobSystem::Destroy()
{
	if (!m_initialized)
		return;

	if (m_threadPool)
	{
		m_threadPool->WaitIdle();
	}

	DestroyThreadContexts();
	m_threadPool.reset();
	m_ctx = nullptr;
	m_initialized = false;

	std::cout << "[KJobSystem] Destroyed" << std::endl;
}

void KJobSystem::CreateThreadContexts()
{
	if (!m_ctx || !m_threadPool)
		return;

	VkDevice device = m_ctx->GetDevice().GetHandle();
	uint32_t queueFamily = m_ctx->GetDevice().GetQueueFamilies()._graphicsFamily;
	uint32_t threadCount = m_threadPool->GetThreadCount();

	m_threadContexts.resize(threadCount);

	for (uint32_t i = 0; i < threadCount; ++i)
	{
		auto& ctx = m_threadContexts[i];

		ctx.commandPool.Init(device, queueFamily,
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		ctx.commandBuffers.resize(COMMAND_BUFFERS_PER_THREAD);
		for (uint32_t j = 0; j < COMMAND_BUFFERS_PER_THREAD; ++j)
		{
			ctx.commandBuffers[j].Allocate(device, ctx.commandPool.GetHandle(), 1,
				VK_COMMAND_BUFFER_LEVEL_SECONDARY);
		}
	}
}

void KJobSystem::DestroyThreadContexts()
{
	for (auto& ctx : m_threadContexts)
	{
		for (auto& cmdBuf : ctx.commandBuffers)
		{
			cmdBuf.Free(ctx.commandPool.GetHandle());
		}
		ctx.commandBuffers.clear();
		ctx.commandPool.Destroy();
	}
	m_threadContexts.clear();
}

KThreadRenderContext& KJobSystem::GetThreadContext(uint32_t threadIndex)
{
	return m_threadContexts[threadIndex];
}

KCommandBuffer& KJobSystem::GetSecondaryBuffer(uint32_t threadIndex, uint32_t frameIndex)
{
	return m_threadContexts[threadIndex].commandBuffers[frameIndex % COMMAND_BUFFERS_PER_THREAD];
}

void KJobSystem::WaitIdle()
{
	if (m_threadPool)
	{
		m_threadPool->WaitIdle();
	}
}

uint32_t KJobSystem::GetThreadCount() const
{
	return m_threadPool ? m_threadPool->GetThreadCount() : 0;
}

} // namespace kEngine
