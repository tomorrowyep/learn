#include "krendergraph.h"
#include <algorithm>
#include <iostream>
#include <queue>
#include <unordered_set>

namespace kEngine
{

void KRenderGraph::Init(KCoreContext* ctx)
{
	m_ctx = ctx;
	m_resourcePool.Init(ctx);
	std::cout << "[KRenderGraph] Initialized" << std::endl;
}

void KRenderGraph::Destroy()
{
	m_resourcePool.Destroy();
	m_resources.clear();
	m_passes.clear();
	m_sortedPassIndices.clear();
	m_ctx = nullptr;
	std::cout << "[KRenderGraph] Destroyed" << std::endl;
}

void KRenderGraph::Reset()
{
	m_resources.clear();
	m_passes.clear();
	m_sortedPassIndices.clear();
	m_compiled = false;
	m_resourceVersion++;
	m_resourcePool.ResetFrame();
}

ResourceHandle KRenderGraph::CreateTexture(const std::string& name, uint32_t width, uint32_t height,
                                           VkFormat format, VkImageUsageFlags usage)
{
	ResourceHandle handle;
	handle.index = static_cast<uint32_t>(m_resources.size());
	handle.version = m_resourceVersion;

	RenderResource resource;
	ResourceDesc desc;
	desc.name = name;
	desc.type = ResourceType::Texture;
	desc.textureDesc.width = width;
	desc.textureDesc.height = height;
	desc.textureDesc.format = format;
	desc.textureDesc.usage = usage;
	desc.imported = false;

	resource.SetDesc(desc);
	m_resources.push_back(resource);

	return handle;
}

ResourceHandle KRenderGraph::ImportTexture(const std::string& name, VkImage image, VkImageView view,
                                           uint32_t width, uint32_t height, VkFormat format)
{
	ResourceHandle handle;
	handle.index = static_cast<uint32_t>(m_resources.size());
	handle.version = m_resourceVersion;

	RenderResource resource;
	ResourceDesc desc;
	desc.name = name;
	desc.type = ResourceType::Texture;
	desc.textureDesc.width = width;
	desc.textureDesc.height = height;
	desc.textureDesc.format = format;
	desc.imported = true;

	PhysicalResource physical;
	physical.image = image;
	physical.imageView = view;

	resource.SetDesc(desc);
	resource.SetPhysical(physical);
	m_resources.push_back(resource);

	return handle;
}

ResourceHandle KRenderGraph::CreateBuffer(const std::string& name, VkDeviceSize size, VkBufferUsageFlags usage)
{
	ResourceHandle handle;
	handle.index = static_cast<uint32_t>(m_resources.size());
	handle.version = m_resourceVersion;

	RenderResource resource;
	ResourceDesc desc;
	desc.name = name;
	desc.type = ResourceType::Buffer;
	desc.bufferDesc.size = size;
	desc.bufferDesc.usage = usage;
	desc.imported = false;

	resource.SetDesc(desc);
	m_resources.push_back(resource);

	return handle;
}

void KRenderGraph::AddPass(const std::string& name, std::function<void(KGraphPass&)> setup)
{
	KGraphPass pass(name);
	setup(pass);
	m_passes.push_back(std::move(pass));
}

RenderResource* KRenderGraph::GetResource(ResourceHandle handle)
{
	if (!handle.IsValid() || handle.index >= m_resources.size())
		return nullptr;

	return &m_resources[handle.index];
}

const RenderResource* KRenderGraph::GetResource(ResourceHandle handle) const
{
	if (!handle.IsValid() || handle.index >= m_resources.size())
		return nullptr;

	return &m_resources[handle.index];
}

void KRenderGraph::Compile()
{
	if (m_passes.empty())
	{
		m_compiled = true;
		return;
	}

	BuildDependencyGraph();
	TopologicalSort();
	CalculateResourceLifetimes();
	AllocateResources();

	m_compiled = true;
}

void KRenderGraph::BuildDependencyGraph()
{
	// Simple implementation: dependencies based on read/write order
	// Pass N depends on Pass M if N reads a resource that M writes
}

void KRenderGraph::TopologicalSort()
{
	// Simple implementation: execute passes in order they were added
	// A more complete implementation would do proper topological sorting
	m_sortedPassIndices.clear();
	for (uint32_t i = 0; i < static_cast<uint32_t>(m_passes.size()); ++i)
	{
		m_passes[i].SetOrder(i);
		m_sortedPassIndices.push_back(i);
	}
}

void KRenderGraph::CalculateResourceLifetimes()
{
	for (uint32_t passIdx = 0; passIdx < static_cast<uint32_t>(m_passes.size()); ++passIdx)
	{
		auto& pass = m_passes[passIdx];

		for (const auto& readHandle : pass.GetReads())
		{
			if (auto* res = GetResource(readHandle))
			{
				if (res->GetFirstPass() == INVALID_RESOURCE_INDEX)
					res->SetFirstPass(passIdx);
				res->SetLastPass(passIdx);
			}
		}

		for (const auto& colorWrite : pass.GetColorWrites())
		{
			if (auto* res = GetResource(colorWrite.handle))
			{
				if (res->GetFirstPass() == INVALID_RESOURCE_INDEX)
					res->SetFirstPass(passIdx);
				res->SetLastPass(passIdx);
			}
		}

		if (pass.HasDepthWrite())
		{
			if (auto* res = GetResource(pass.GetDepthWrite().handle))
			{
				if (res->GetFirstPass() == INVALID_RESOURCE_INDEX)
					res->SetFirstPass(passIdx);
				res->SetLastPass(passIdx);
			}
		}
	}
}

void KRenderGraph::AllocateResources()
{
	for (auto& resource : m_resources)
	{
		const auto& desc = resource.GetDesc();

		if (desc.imported)
			continue;

		if (desc.type == ResourceType::Texture)
		{
			TextureDesc texDesc = desc.textureDesc;

			if (texDesc.usage == 0)
			{
				bool isDepth = (texDesc.format == VK_FORMAT_D32_SFLOAT ||
				                texDesc.format == VK_FORMAT_D24_UNORM_S8_UINT ||
				                texDesc.format == VK_FORMAT_D16_UNORM);

				if (isDepth)
				{
					texDesc.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
					                VK_IMAGE_USAGE_SAMPLED_BIT;
				}
				else
				{
					texDesc.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
					                VK_IMAGE_USAGE_SAMPLED_BIT;
				}
			}

			PhysicalResource physical = m_resourcePool.AcquireTexture(texDesc);
			resource.SetPhysical(physical);
		}
		else if (desc.type == ResourceType::Buffer)
		{
			PhysicalResource physical = m_resourcePool.AcquireBuffer(desc.bufferDesc);
			resource.SetPhysical(physical);
		}
	}
}

void KRenderGraph::Execute(VkCommandBuffer cmd, KJobSystem* jobSystem,
                           uint32_t frameIndex, uint32_t imageIndex)
{
	if (!m_compiled)
	{
		std::cerr << "[KRenderGraph] Graph not compiled" << std::endl;
		return;
	}

	RenderContext ctx;
	ctx.commandBuffer = cmd;
	ctx.graph = this;
	ctx.jobSystem = jobSystem;
	ctx.frameIndex = frameIndex;
	ctx.imageIndex = imageIndex;

	for (uint32_t passIdx : m_sortedPassIndices)
	{
		auto& pass = m_passes[passIdx];
		ctx.passIndex = passIdx;

		for (const auto& colorWrite : pass.GetColorWrites())
		{
			if (auto* res = GetResource(colorWrite.handle))
			{
				if (res->GetDesc().type == ResourceType::Texture)
				{
					InsertBarrier(cmd, *res, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
				}
			}
		}

		if (pass.HasDepthWrite())
		{
			if (auto* res = GetResource(pass.GetDepthWrite().handle))
			{
				if (res->GetDesc().type == ResourceType::Texture)
				{
					InsertBarrier(cmd, *res, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					              VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
				}
			}
		}

		pass.Execute(ctx);

		for (const auto& readHandle : pass.GetReads())
		{
			if (auto* res = GetResource(readHandle))
			{
				if (res->GetDesc().type == ResourceType::Texture)
				{
					InsertBarrier(cmd, *res, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					              VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					              VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
				}
			}
		}
	}
}

void KRenderGraph::InsertBarrier(VkCommandBuffer cmd, RenderResource& resource, VkImageLayout newLayout,
                                 VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)
{
	VkImageLayout oldLayout = resource.GetCurrentLayout();

	if (oldLayout == newLayout)
		return;

	const auto& physical = resource.GetPhysical();
	if (physical.image == VK_NULL_HANDLE)
		return;

	const auto& desc = resource.GetDesc();
	bool isDepth = (desc.textureDesc.format == VK_FORMAT_D32_SFLOAT ||
	                desc.textureDesc.format == VK_FORMAT_D24_UNORM_S8_UINT ||
	                desc.textureDesc.format == VK_FORMAT_D16_UNORM);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = physical.image;
	barrier.subresourceRange.aspectMask = isDepth ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	switch (oldLayout)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		barrier.srcAccessMask = 0;
		break;
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	default:
		barrier.srcAccessMask = 0;
		break;
	}

	switch (newLayout)
	{
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		break;
	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		barrier.dstAccessMask = 0;
		break;
	default:
		barrier.dstAccessMask = 0;
		break;
	}

	vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	resource.SetCurrentLayout(newLayout);
}

} // namespace kEngine
