#ifndef __KFORWARD_RENDERER_H__
#define __KFORWARD_RENDERER_H__

#include "renderer/include/irenderer.h"
#include "context/klogiccontext.h"
#include "core/renderPass/krenderpass.h"
#include "core/objects/kframebuffer.h"

class KForwardRenderer : public IKRenderer
{
public:
	bool Init(KLogicContext& ctx) override;
	void Destroy() override;

	void OnSwapchainRecreated(KLogicContext& ctx) override;

	void Record(
		KLogicContext& ctx,
		KFrameResources& frame,
		uint32_t imageIndex) override;

private:
	void _createRenderPass(KLogicContext& core);
	void _createFramebuffers(KLogicContext& core);
	void _destroyFramebuffers();

private:
	KRenderPass  m_renderPass;
	std::vector<KFramebuffer> m_framebuffers;
};

#endif // !__KFORWARD_RENDERER_H__
