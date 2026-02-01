#pragma once

#include "../common.h"
#include "renderdata.h"
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
#include "core/commands/kcommandbuffer.h"

namespace kEngine
{

class ResourceManager;

class ForwardPass
{
public:
	ForwardPass() = default;
	~ForwardPass() = default;

	bool init(KCoreContext& ctx, KSwapchain& swapchain, ResourceManager& resources);
	void destroy();
	void onResize(KCoreContext& ctx, KSwapchain& swapchain);

	void render(VkCommandBuffer cmd, uint32_t imageIndex, const RenderScene& renderScene, ResourceManager& resources);

private:
	bool createRenderPass(VkDevice device, VkFormat colorFormat, VkFormat depthFormat);
	bool createDepthResources(VkDevice device, VkPhysicalDevice physDevice, uint32_t w, uint32_t h);
	bool createFramebuffers(VkDevice device, KSwapchain& swapchain);
	bool createCameraDescriptorSetLayout(VkDevice device);
	bool createPipeline(VkDevice device, VkExtent2D extent, VkDescriptorSetLayout materialLayout);
	bool createCameraUBO(VkDevice device, VkPhysicalDevice physDevice);
	bool createCameraDescriptorPool(VkDevice device);
	bool createCameraDescriptorSet(VkDevice device);
	void destroyFramebuffers();
	void destroyDepthResources();

private:
	KRenderPass              m_renderPass;
	KImage                   m_depthImage;
	KImageView               m_depthImageView;
	std::vector<KFramebuffer> m_framebuffers;

	KShaderModule            m_vertShader;
	KShaderModule            m_fragShader;
	KDescriptorSetLayout     m_cameraDescriptorSetLayout;
	KPipelineLayout          m_pipelineLayout;
	KPipeline                m_pipeline;

	KDescriptorPool          m_cameraDescriptorPool;
	KDescriptorSet           m_cameraDescriptorSet;
	KBuffer                  m_cameraUBO;

	VkDevice                 m_device = VK_NULL_HANDLE;
	VkExtent2D               m_extent{};
	bool                     m_initialized = false;
};

} // namespace kEngine
