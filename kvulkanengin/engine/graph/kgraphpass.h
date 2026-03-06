#ifndef __KGRAPHPASS_H__
#define __KGRAPHPASS_H__

#include "krenderresource.h"
#include <functional>
#include <vector>
#include <string>

namespace kEngine
{

class KRenderGraph;
class KJobSystem;

struct RenderContext
{
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	KRenderGraph*   graph = nullptr;
	KJobSystem*     jobSystem = nullptr;
	uint32_t        passIndex = 0;
	uint32_t        frameIndex = 0;
	uint32_t        imageIndex = 0;
};

struct ColorAttachment
{
	ResourceHandle handle;
	uint32_t       slot = 0;
	VkClearValue   clearValue = {};
	bool           clear = true;
};

struct DepthAttachment
{
	ResourceHandle handle;
	VkClearValue   clearValue = {};
	bool           clear = true;
};

class KGraphPass
{
public:
	KGraphPass() = default;
	KGraphPass(const std::string& name);
	~KGraphPass() = default;

	void SetName(const std::string& name) { m_name = name; }
	const std::string& GetName() const { return m_name; }

	void ReadTexture(ResourceHandle handle);
	void WriteColor(ResourceHandle handle, uint32_t slot = 0, bool clear = true);
	void WriteDepth(ResourceHandle handle, bool clear = true);

	void SetExecuteCallback(std::function<void(RenderContext&)> callback);

	const std::vector<ResourceHandle>& GetReads() const { return m_reads; }
	const std::vector<ColorAttachment>& GetColorWrites() const { return m_colorWrites; }
	const DepthAttachment& GetDepthWrite() const { return m_depthWrite; }
	bool HasDepthWrite() const { return m_depthWrite.handle.IsValid(); }

	void Execute(RenderContext& ctx);

	void SetOrder(uint32_t order) { m_order = order; }
	uint32_t GetOrder() const { return m_order; }

private:
	std::string                          m_name;
	std::vector<ResourceHandle>          m_reads;
	std::vector<ColorAttachment>         m_colorWrites;
	DepthAttachment                      m_depthWrite;
	std::function<void(RenderContext&)>  m_executeCallback;
	uint32_t                             m_order = 0;
};

} // namespace kEngine

#endif // !__KGRAPHPASS_H__
