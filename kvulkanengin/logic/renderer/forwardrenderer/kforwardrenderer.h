#ifndef __KFORWARD_RENDERER_H__
#define __KFORWARD_RENDERER_H__

#include "renderer/include/irenderer.h"
#include "context/klogiccontext.h"
#include "core/renderPass/krenderpass.h"
#include "core/objects/kframebuffer.h"
#include "core/pipeline/kpipeline.h"
#include "core/pipeline/kpipelinelayout.h"
#include "core/pipeline/kshadermodule.h"

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
	bool _createRenderPass(KLogicContext& core);
	bool _createPipeline(KLogicContext& core);
	void _destroyPipeline();
	bool _createFramebuffers(KLogicContext& core);
	void _destroyFramebuffers();

private:
	KRenderPass  m_renderPass;
	KShaderModule m_vertShader;
	KShaderModule m_fragShader;
	KPipelineLayout m_pipelineLayout;
	KPipeline m_pipeline;
	std::vector<KFramebuffer> m_framebuffers;
};

#endif // !__KFORWARD_RENDERER_H__
