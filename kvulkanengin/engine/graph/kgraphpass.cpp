#include "kgraphpass.h"

namespace kEngine
{

KGraphPass::KGraphPass(const std::string& name)
	: m_name(name)
{
}

void KGraphPass::ReadTexture(ResourceHandle handle)
{
	if (handle.IsValid())
	{
		m_reads.push_back(handle);
	}
}

void KGraphPass::WriteColor(ResourceHandle handle, uint32_t slot, bool clear)
{
	if (handle.IsValid())
	{
		ColorAttachment att;
		att.handle = handle;
		att.slot = slot;
		att.clear = clear;
		att.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		m_colorWrites.push_back(att);
	}
}

void KGraphPass::WriteDepth(ResourceHandle handle, bool clear)
{
	if (handle.IsValid())
	{
		m_depthWrite.handle = handle;
		m_depthWrite.clear = clear;
		m_depthWrite.clearValue.depthStencil = {1.0f, 0};
	}
}

void KGraphPass::SetExecuteCallback(std::function<void(RenderContext&)> callback)
{
	m_executeCallback = std::move(callback);
}

void KGraphPass::Execute(RenderContext& ctx)
{
	if (m_executeCallback)
	{
		m_executeCallback(ctx);
	}
}

} // namespace kEngine
