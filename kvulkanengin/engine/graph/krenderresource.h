#ifndef __KRENDERRESOURCE_H__
#define __KRENDERRESOURCE_H__

#include <vulkan.h>
#include <string>
#include <cstdint>

namespace kEngine
{

static constexpr uint32_t INVALID_RESOURCE_INDEX = 0xFFFFFFFF;

struct ResourceHandle
{
	uint32_t index = INVALID_RESOURCE_INDEX;
	uint32_t version = 0;

	bool IsValid() const { return index != INVALID_RESOURCE_INDEX; }

	bool operator==(const ResourceHandle& other) const
	{
		return index == other.index && version == other.version;
	}

	bool operator!=(const ResourceHandle& other) const
	{
		return !(*this == other);
	}
};

enum class ResourceType
{
	Texture,
	Buffer
};

struct TextureDesc
{
	uint32_t          width = 0;
	uint32_t          height = 0;
	VkFormat          format = VK_FORMAT_UNDEFINED;
	VkImageUsageFlags usage = 0;
	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
};

struct BufferDesc
{
	VkDeviceSize       size = 0;
	VkBufferUsageFlags usage = 0;
};

struct ResourceDesc
{
	std::string   name;
	ResourceType  type = ResourceType::Texture;
	TextureDesc   textureDesc;
	BufferDesc    bufferDesc;
	bool          imported = false;
};

struct PhysicalResource
{
	VkImage       image = VK_NULL_HANDLE;
	VkImageView   imageView = VK_NULL_HANDLE;
	VkBuffer      buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
};

class RenderResource
{
public:
	RenderResource() = default;
	~RenderResource() = default;

	void SetDesc(const ResourceDesc& desc) { m_desc = desc; }
	const ResourceDesc& GetDesc() const { return m_desc; }

	void SetPhysical(const PhysicalResource& physical) { m_physical = physical; }
	const PhysicalResource& GetPhysical() const { return m_physical; }

	void SetFirstPass(uint32_t pass) { m_firstPass = pass; }
	void SetLastPass(uint32_t pass) { m_lastPass = pass; }
	uint32_t GetFirstPass() const { return m_firstPass; }
	uint32_t GetLastPass() const { return m_lastPass; }

	VkImageLayout GetCurrentLayout() const { return m_currentLayout; }
	void SetCurrentLayout(VkImageLayout layout) { m_currentLayout = layout; }

private:
	ResourceDesc     m_desc;
	PhysicalResource m_physical;
	uint32_t         m_firstPass = INVALID_RESOURCE_INDEX;
	uint32_t         m_lastPass = INVALID_RESOURCE_INDEX;
	VkImageLayout    m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};

} // namespace kEngine

#endif // !__KRENDERRESOURCE_H__
