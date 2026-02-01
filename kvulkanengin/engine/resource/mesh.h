#pragma once

#include "../common.h"
#include <array>
#include "core/objects/kbuffer.h"
#include <vulkan/vulkan.h>

namespace kEngine
{

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;

	bool operator==(const Vertex& other) const
	{
		return position == other.position &&
		       normal == other.normal &&
		       texCoord == other.texCoord;
	}

	static VkVertexInputBindingDescription getBindingDescription()
	{
		return {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
	{
		return {{
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
			{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
			{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)}
		}};
	}
};

class Mesh
{
public:
	Mesh() = default;
	~Mesh() = default;

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;

	Mesh(Mesh&&) = default;
	Mesh& operator=(Mesh&&) = default;

	std::vector<Vertex>   vertices;
	std::vector<uint32_t> indices;

	KBuffer vertexBuffer;
	KBuffer indexBuffer;

	bool isUploaded() const { return m_uploaded; }
	void setUploaded(bool v) { m_uploaded = v; }

	uint32_t getVertexCount() const { return static_cast<uint32_t>(vertices.size()); }
	uint32_t getIndexCount() const { return static_cast<uint32_t>(indices.size()); }

private:
	bool m_uploaded = false;
};

} // namespace kEngine

namespace std
{
	template<>
	struct hash<kEngine::Vertex>
	{
		size_t operator()(const kEngine::Vertex& v) const
		{
			size_t h = 0;
			auto hashCombine = [&h](size_t val)
			{
				h ^= val + 0x9e3779b9 + (h << 6) + (h >> 2);
			};
			hashCombine(hash<float>()(v.position.x));
			hashCombine(hash<float>()(v.position.y));
			hashCombine(hash<float>()(v.position.z));
			hashCombine(hash<float>()(v.normal.x));
			hashCombine(hash<float>()(v.normal.y));
			hashCombine(hash<float>()(v.normal.z));
			hashCombine(hash<float>()(v.texCoord.x));
			hashCombine(hash<float>()(v.texCoord.y));
			return h;
		}
	};
}
