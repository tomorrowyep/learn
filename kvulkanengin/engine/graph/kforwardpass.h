#ifndef __KFORWARDPASS_H__
#define __KFORWARDPASS_H__

#include "krendergraph.h"
#include "../render/renderdata.h"
#include "core/renderpass/krenderpass.h"
#include "core/objects/kframebuffer.h"
#include "core/objects/kbuffer.h"
#include "core/objects/kimage.h"
#include "core/objects/kimageview.h"
#include "core/pipeline/kpipeline.h"
#include "core/pipeline/kpipelinelayout.h"
#include "core/pipeline/kshadermodule.h"
#include "core/descriptor/kdescriptorpool.h"
#include "core/descriptor/kdescriptorset.h"
#include "core/descriptor/kdescriptorsetlayout.h"
#include "core/context/kcorecontext.h"
#include "core/context/kswapchain.h"

namespace kEngine
{

class ResourceManager;
class KJobSystem;

class KForwardPass
{
public:
	KForwardPass() = default;
	~KForwardPass() = default;

	bool Init(KCoreContext& ctx, KSwapchain& swapchain, ResourceManager& resources);
	void Destroy();
	void OnResize(KCoreContext& ctx, KSwapchain& swapchain);

	void SetupPass(KRenderGraph& graph, uint32_t imageIndex);
	void Execute(RenderContext& ctx, const RenderScene& renderScene, ResourceManager& resources);

	VkRenderPass GetRenderPassHandle() const { return m_renderPass.GetHandle(); }
	VkFramebuffer GetFramebuffer(uint32_t imageIndex) const { return m_framebuffers[imageIndex].GetHandle(); }
	VkExtent2D GetExtent() const { return m_extent; }

	void SetParallelThreshold(uint32_t threshold) { m_parallelThreshold = threshold; }

private:
	bool CreateRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat);
	bool CreateDepthResources(VkDevice device, VkPhysicalDevice physDevice, uint32_t w, uint32_t h);
	bool CreateFramebuffers(VkDevice device, KSwapchain& swapchain);
	bool CreateCameraDescriptorSetLayout(VkDevice device);
	bool CreatePipeline(VkDevice device, VkExtent2D extent, VkDescriptorSetLayout materialLayout);
	bool CreateCameraUBO(VkDevice device, VkPhysicalDevice physDevice);
	bool CreateCameraDescriptorPool(VkDevice device);
	bool CreateCameraDescriptorSet(VkDevice device);
	void DestroyFramebuffers();
	void DestroyDepthResources();

	void ExecuteSingleThread(VkCommandBuffer cmd, const RenderScene& renderScene, ResourceManager& resources);
	void ExecuteParallel(RenderContext& ctx, const RenderScene& renderScene, ResourceManager& resources);
	void SetupCommandState(VkCommandBuffer cmd);
	void RecordRenderItem(VkCommandBuffer cmd, const RenderItem& item, ResourceManager& resources);

private:
	KRenderPass               m_renderPass;
	KImage                    m_depthImage;
	KImageView                m_depthImageView;
	std::vector<KFramebuffer> m_framebuffers;

	KShaderModule             m_vertShader;
	KShaderModule             m_fragShader;
	KDescriptorSetLayout      m_cameraDescriptorSetLayout;
	KPipelineLayout           m_pipelineLayout;
	KPipeline                 m_pipeline;

	KDescriptorPool           m_cameraDescriptorPool;
	KDescriptorSet            m_cameraDescriptorSet;
	KBuffer                   m_cameraUBO;

	ResourceHandle            m_colorTarget;
	ResourceHandle            m_depthTarget;
	uint32_t                  m_currentImageIndex = 0;

	VkDevice                  m_device = VK_NULL_HANDLE;
	VkExtent2D                m_extent{};
	bool                      m_initialized = false;
	uint32_t                  m_parallelThreshold = 50;
};

} // namespace kEngine

#endif // !__KFORWARDPASS_H__
