#ifndef __KRENDERGRAPH_H__
#define __KRENDERGRAPH_H__

#include "krenderresource.h"
#include "kgraphpass.h"
#include "kresourcepool.h"
#include "core/context/kcorecontext.h"
#include <vector>
#include <functional>

namespace kEngine
{

class KRenderGraph
{
public:
	KRenderGraph() = default;
	~KRenderGraph() = default;

	void Init(KCoreContext* ctx);
	void Destroy();

	ResourceHandle CreateTexture(const std::string& name, uint32_t width, uint32_t height,
	                             VkFormat format, VkImageUsageFlags usage = 0);

	ResourceHandle ImportTexture(const std::string& name, VkImage image, VkImageView view,
	                             uint32_t width, uint32_t height, VkFormat format);

	ResourceHandle CreateBuffer(const std::string& name, VkDeviceSize size, VkBufferUsageFlags usage);

	void AddPass(const std::string& name, std::function<void(KGraphPass&)> setup);

	void Compile();
	void Execute(VkCommandBuffer cmd, KJobSystem* jobSystem = nullptr,
	             uint32_t frameIndex = 0, uint32_t imageIndex = 0);
	void Reset();

	RenderResource* GetResource(ResourceHandle handle);
	const RenderResource* GetResource(ResourceHandle handle) const;

	KCoreContext* GetContext() { return m_ctx; }

private:
	void BuildDependencyGraph();
	void TopologicalSort();
	void CalculateResourceLifetimes();
	void AllocateResources();
	void InsertBarrier(VkCommandBuffer cmd, RenderResource& resource, VkImageLayout newLayout,
	                   VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);

private:
	KCoreContext*                m_ctx = nullptr;
	KResourcePool                m_resourcePool;

	std::vector<RenderResource>  m_resources;
	std::vector<KGraphPass>      m_passes;
	std::vector<uint32_t>        m_sortedPassIndices;

	uint32_t                     m_resourceVersion = 0;
	bool                         m_compiled = false;
};

} // namespace kEngine

#endif // !__KRENDERGRAPH_H__
