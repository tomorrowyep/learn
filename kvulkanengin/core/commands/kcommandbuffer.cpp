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
	
	// 重置第一个命令缓冲区（通常只使用一个）
	VkResult res = vkResetCommandBuffer(m_buffers[0], 0);
	assert(res == VK_SUCCESS);
}

void KCommandBuffer::Begin()
{
	assert(!m_buffers.empty());
	assert(m_device != VK_NULL_HANDLE);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // 可以设置为 VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT 等
	beginInfo.pInheritanceInfo = nullptr; // 仅在 Secondary 级别时使用

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
	// 分配临时命令缓冲区
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO; // 命令缓冲区分配信息
	allocInfo.commandPool = m_pool; // 命令池
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // 命令缓冲区级别
	allocInfo.commandBufferCount = 1; // 命令缓冲区数量

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

	// 开始记录命令缓冲区
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO; // 命令缓冲区开始信息
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // 命令缓冲区使用一次性提交

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void KCommandBuffer::EndSingleCommand(VkCommandBuffer commandBuffer, VkQueue queue)
{
	// 结束记录命令缓冲区
	vkEndCommandBuffer(commandBuffer);

	// 提交命令缓冲区到队列
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO; // 提交信息
	submitInfo.commandBufferCount = 1; // 命令缓冲区数量
	submitInfo.pCommandBuffers = &commandBuffer; // 命令缓冲区

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE); // 提交到队列
	vkQueueWaitIdle(queue); // 等待队列空闲

	// 释放命令缓冲区
	vkFreeCommandBuffers(m_device, m_pool, 1, &commandBuffer);
}
