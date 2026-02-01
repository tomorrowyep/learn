#ifndef __KCOMMANDBUFFER_H__
#define __KCOMMANDBUFFER_H__

#include <vulkan.h>
#include <vector>

class KCommandBuffer
{
public:
	KCommandBuffer() = default;
	~KCommandBuffer();

	KCommandBuffer(const KCommandBuffer&) = delete;
	KCommandBuffer& operator=(const KCommandBuffer&) = delete;

	KCommandBuffer(KCommandBuffer&& other) noexcept;
	KCommandBuffer& operator=(KCommandBuffer&& other) noexcept;

	bool Allocate(VkDevice device,
		VkCommandPool pool, uint32_t count = 1,
		VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	void Free(VkCommandPool pool);

	void Reset();
	void Begin();
	void End();

	VkCommandBuffer BeginSingleCommand();
	void EndSingleCommand(VkCommandBuffer commandBuffer, VkQueue queue);

	VkCommandBuffer GetHandle(size_t index = 0) const { return m_buffers[index]; }
	std::vector<VkCommandBuffer>& GetHandles() { return m_buffers; }
	VkCommandPool GetPool() const { return m_pool; }

	struct KSingalRecordhelper
	{
		KSingalRecordhelper(KCommandBuffer* cmds, VkQueue queue)
			: m_pCmds(cmds)
			, m_queue(queue)
		{
			m_cmd = m_pCmds->BeginSingleCommand();
		}

		~KSingalRecordhelper()
		{
			m_pCmds->EndSingleCommand(m_cmd, m_queue);
		}

	private:
		KCommandBuffer* m_pCmds = nullptr;
		VkCommandBuffer m_cmd = VK_NULL_HANDLE;
		VkQueue m_queue = VK_NULL_HANDLE;
	};

private:
	VkDevice                    m_device{ VK_NULL_HANDLE };
	VkCommandPool               m_pool{ VK_NULL_HANDLE };
	std::vector<VkCommandBuffer> m_buffers;
};
#endif // !__KCOMMANDBUFFER_H__
