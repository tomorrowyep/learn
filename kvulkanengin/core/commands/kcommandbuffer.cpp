#include "kcommandbuffer.h"
#include <cassert>

KCommandBuffer::~KCommandBuffer()
{
	Free(m_pool);
}

bool KCommandBuffer::Allocate(VkDevice device, VkCommandPool pool, uint32_t count, VkCommandBufferLevel level)
{
	assert(device != VK_NULL_HANDLE && pool != VK_NULL_HANDLE);

	m_device = device;
	m_pool = pool;
	m_buffers.resize(count);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = pool;
	allocInfo.level = level;
	allocInfo.commandBufferCount = count;

	if (vkAllocateCommandBuffers(m_device, &allocInfo, m_buffers.data()) != VK_SUCCESS)
		return false;

	return true;
}

void KCommandBuffer::Free(VkCommandPool pool)
{
	if (!m_buffers.empty() && pool != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(m_device, pool, static_cast<uint32_t>(m_buffers.size()), m_buffers.data());
		m_buffers.clear();
	}
}

void KCommandBuffer::Reset()
{
	assert(!m_buffers.empty());

	VkResult res = vkResetCommandBuffer(m_buffers[0], 0);
	assert(res == VK_SUCCESS);
}

void KCommandBuffer::Begin()
{
	assert(!m_buffers.empty());
	assert(m_device != VK_NULL_HANDLE);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;

	VkResult res = vkBeginCommandBuffer(m_buffers[0], &beginInfo);
	assert(res == VK_SUCCESS);
}

void KCommandBuffer::BeginSecondary(VkRenderPass renderPass, uint32_t subpass, VkFramebuffer framebuffer)
{
	assert(!m_buffers.empty());
	assert(m_device != VK_NULL_HANDLE);

	VkCommandBufferInheritanceInfo inheritance{};
	inheritance.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	inheritance.renderPass = renderPass;
	inheritance.subpass = subpass;
	inheritance.framebuffer = framebuffer;

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT |
	                  VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	beginInfo.pInheritanceInfo = &inheritance;

	VkResult res = vkBeginCommandBuffer(m_buffers[0], &beginInfo);
	assert(res == VK_SUCCESS);
}

void KCommandBuffer::End()
{
	assert(!m_buffers.empty());

	VkResult res = vkEndCommandBuffer(m_buffers[0]);
	assert(res == VK_SUCCESS);
}

KCommandBuffer::KCommandBuffer(KCommandBuffer&& other) noexcept
{
	*this = std::move(other);
}

KCommandBuffer& KCommandBuffer::operator=(KCommandBuffer&& other) noexcept
{
	if (this != &other)
	{
		Free(m_pool);

		m_device = other.m_device;
		m_pool = other.m_pool;
		m_buffers = std::move(other.m_buffers);

		other.m_device = VK_NULL_HANDLE;
		other.m_pool = VK_NULL_HANDLE;
	}

	return *this;
}

VkCommandBuffer KCommandBuffer::BeginSingleCommand()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_pool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void KCommandBuffer::EndSingleCommand(VkCommandBuffer commandBuffer, VkQueue queue)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(m_device, m_pool, 1, &commandBuffer);
}
