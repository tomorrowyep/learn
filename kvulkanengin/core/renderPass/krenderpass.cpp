#include "krenderpass.h"
#include "renderpassdesc.h"

KRenderPass::~KRenderPass()
{
	Destroy();
}

KRenderPass::KRenderPass(KRenderPass&& other) noexcept
{
	*this = std::move(other);
}

KRenderPass& KRenderPass::operator=(KRenderPass&& other) noexcept
{
	if (this != &other)
	{
		Destroy();

		m_device = other.m_device;
		m_renderPass = other.m_renderPass;

		other.m_device = VK_NULL_HANDLE;
		other.m_renderPass = VK_NULL_HANDLE;
	}

	return *this;
}

bool KRenderPass::Init(VkDevice device, const KRenderPassCreateDesc& desc)
{
	// 如果已经初始化过，先销毁旧的资源
	Destroy();
	
	m_device = device;

	/* ---------- 1. Attachments ---------- */
	std::vector<VkAttachmentDescription> attachments;
	attachments.reserve(desc.attachments.size());

	for (const auto& a : desc.attachments)
	{
		VkAttachmentDescription ad{};
		ad.format = a.format;
		ad.samples = a.samples;
		ad.loadOp = a.loadOp;
		ad.storeOp = a.storeOp;
		ad.stencilLoadOp = a.stencilLoadOp;
		ad.stencilStoreOp = a.stencilStoreOp;
		ad.initialLayout = a.initialLayout;
		ad.finalLayout = a.finalLayout;

		attachments.push_back(ad);
	}

	/* ---------- 2. Subpasses ---------- */
	std::vector<VkSubpassDescription> subpasses;
	subpasses.reserve(desc.subpasses.size());

	std::vector<std::vector<VkAttachmentReference>> colorRefs;
	std::vector<std::vector<VkAttachmentReference>> resolveRefs;
	std::vector<VkAttachmentReference> depthRefs;

	colorRefs.resize(desc.subpasses.size());
	resolveRefs.resize(desc.subpasses.size());
	depthRefs.resize(desc.subpasses.size());

	for (size_t i = 0; i < desc.subpasses.size(); ++i)
	{
		const auto& sp = desc.subpasses[i];

		// color attachments
		for (const auto& c : sp.colorAttachments)
		{
			colorRefs[i].push_back({
				c.attachmentIndex,
				c.layout
				});
		}

		// resolve attachments
		for (const auto& r : sp.resolveAttachments)
		{
			resolveRefs[i].push_back({
				r.attachmentIndex,
				r.layout
				});
		}

		// depth attachment
		VkAttachmentReference* depthRefPtr = nullptr;
		if (sp.depthStencilAttachment.has_value())
		{
			depthRefs[i] = {
				sp.depthStencilAttachment->attachmentIndex,
				sp.depthStencilAttachment->layout
			};
			depthRefPtr = &depthRefs[i];
		}

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = sp.pipelineBindPoint;
		subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs[i].size());
		subpass.pColorAttachments = colorRefs[i].empty() ? nullptr : colorRefs[i].data();
		subpass.pResolveAttachments = resolveRefs[i].empty() ? nullptr : resolveRefs[i].data();
		subpass.pDepthStencilAttachment = depthRefPtr;

		subpasses.push_back(subpass);
	}

	/* ---------- 3. Dependencies ---------- */
	std::vector<VkSubpassDependency> dependencies;
	dependencies.reserve(desc.dependencies.size());

	for (const auto& d : desc.dependencies)
	{
		VkSubpassDependency dep{};
		dep.srcSubpass = d.srcSubpass;
		dep.dstSubpass = d.dstSubpass;
		dep.srcStageMask = d.srcStageMask;
		dep.dstStageMask = d.dstStageMask;
		dep.srcAccessMask = d.srcAccessMask;
		dep.dstAccessMask = d.dstAccessMask;
		dep.dependencyFlags = d.dependencyFlags;

		dependencies.push_back(dep);
	}

	/* ---------- 4. Create RenderPass ---------- */
	VkRenderPassCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = static_cast<uint32_t>(attachments.size());
	info.pAttachments = attachments.data();
	info.subpassCount = static_cast<uint32_t>(subpasses.size());
	info.pSubpasses = subpasses.data();
	info.dependencyCount = static_cast<uint32_t>(dependencies.size());
	info.pDependencies = dependencies.data();

	if (vkCreateRenderPass(device, &info, nullptr, &m_renderPass) != VK_SUCCESS)
		return false;

	return true;
}


void KRenderPass::Destroy()
{
	// 必须在设备销毁之前销毁 renderpass，否则验证层会报错
	// 只有当设备和 renderpass 都有效时才真正销毁
	if (m_renderPass != VK_NULL_HANDLE && m_device != VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(m_device, m_renderPass, nullptr);
		m_renderPass = VK_NULL_HANDLE;
	}
	// 注意：如果设备已经无效，我们无法销毁 renderpass，但验证层会报错
	// 这通常意味着销毁顺序有问题，应该在设备销毁前显式调用 Destroy()
	
	m_device = VK_NULL_HANDLE;
}

