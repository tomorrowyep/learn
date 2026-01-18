#include "kframeresources.h"

KFrameResources::~KFrameResources()
{
	Destroy();
}

KFrameResources::KFrameResources(KFrameResources&& other) noexcept
{
	*this = std::move(other);
}

KFrameResources& KFrameResources::operator=(KFrameResources&& other) noexcept
{
	if (this != &other)
	{
		Destroy();

		m_device = other.m_device;
		m_cmdPool = std::move(other.m_cmdPool);
		m_cmdBuffer = std::move(other.m_cmdBuffer);
		m_inFlightFence = std::move(other.m_inFlightFence);
		m_imageAvailable = std::move(other.m_imageAvailable);
		m_renderFinished = std::move(other.m_renderFinished);

		other.m_device = nullptr;
	}

	return *this;
}

bool KFrameResources::Init(KDevice& device)
{
	m_device = &device;

	// 1. Command Pool，通常使用 Graphics Queue Family
	if (!m_cmdPool.Init(device.GetHandle(), m_device->GetQueueFamilies()._graphicsFamily))
		return false;

	// 2. Command Buffer，1 个 Primary
	if (!m_cmdBuffer.Allocate(
		device.GetHandle(),
		m_cmdPool.GetHandle()))
		return false;

	// 3. Fence，初始为 signaled，第一帧等待时不会阻塞
	if (!m_inFlightFence.Init(device.GetHandle(), true))
		return false;

	// 4. Semaphores
	if (!m_imageAvailable.Init(device.GetHandle()))
		return false;

	if (!m_renderFinished.Init(device.GetHandle()))
		return false;

	return true;
}

void KFrameResources::Destroy()
{
	if (!m_device)
		return;

	m_device->WaitIdle();

	m_renderFinished.Destroy();
	m_imageAvailable.Destroy();
	m_inFlightFence.Destroy();

	m_cmdBuffer.Free(m_cmdPool.GetHandle());
	m_cmdPool.Destroy();

	m_device = {};
}
