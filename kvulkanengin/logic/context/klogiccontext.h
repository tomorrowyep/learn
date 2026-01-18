#ifndef __KLOGICCONTEXT_H__
#define __KLOGICCONTEXT_H__

#include <vector>
#include "core/context/kcorecontext.h"
#include "core/context/kswapchain.h"
#include "frame/kframeresources.h"

class IKRenderer;
class IKWindow;

class KLogicContext
{
public:
	bool Init(KCoreContext & core, const SwapchainCreateDesc& desc);
	void Destroy();

	void SetRenderer(IKRenderer* renderer);
	void SetWindow(IKWindow* pWin);

	KCoreContext* getCoreCtx() { return m_core; }
	KSwapchain* GetSwapchain() { return &m_swapchain; }

	bool BeginFrame();
	void Render();
	void EndFrame();

private:
	void _recreateSwapchain();

private:
	KCoreContext* m_core = nullptr;
	KSwapchain m_swapchain;

	SwapchainCreateDesc m_swapChainDesc;

	std::vector<KFrameResources> m_frames;
	uint32_t m_maxFrames = 0;
	uint32_t m_currentFrame = 0;
	uint32_t m_imageIndex = 0;

	IKRenderer* m_pRenderer = nullptr;
	IKWindow* m_pWin = nullptr;
};
#endif // !__KLOGICCONTEXT_H__



