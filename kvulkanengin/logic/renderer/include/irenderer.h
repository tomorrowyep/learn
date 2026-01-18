#ifndef __IKRENDER_H__
#define __IKRENDER_H__

#include "context/klogiccontext.h"
#include "frame/kframeresources.h"

class IKRenderer
{
public:
	virtual ~IKRenderer() = default;

	virtual bool Init(KLogicContext& ctx) = 0;
	virtual void Destroy() = 0;

	// swapchain 重建时调用
	virtual void OnSwapchainRecreated(KLogicContext& ctx) = 0;

	// 每帧
	virtual void Record(
		KLogicContext& ctx,
		KFrameResources& frame,
		uint32_t imageIndex) = 0;
};
#endif // !__IKRENDER_H__
