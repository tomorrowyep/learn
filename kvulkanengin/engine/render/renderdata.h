#pragma once

#include "../common.h"
#include <vulkan/vulkan.h>
#include <algorithm>

namespace kEngine
{

// GPU Uniform data
struct alignas(16) CameraUBO
{
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 viewProjection;
	glm::vec4 position;
};

struct alignas(16) ObjectUBO
{
	glm::mat4 model;
	glm::mat4 normalMatrix;
};

// Render item
struct RenderItem
{
	uint32_t  meshID;
	uint32_t  materialID;
	glm::mat4 transform;
	float     sortKey;
};

// Render scene
struct RenderScene
{
	CameraUBO               cameraData;
	bool                    hasCamera = false;
	std::vector<RenderItem> opaqueItems;
	std::vector<RenderItem> transparentItems;

	void clear()
	{
		hasCamera = false;
		opaqueItems.clear();
		transparentItems.clear();
	}

	void sortOpaqueByDistance()
	{
		std::sort(opaqueItems.begin(), opaqueItems.end(),
			[](const RenderItem& a, const RenderItem& b)
			{
				return a.sortKey < b.sortKey;
			});
	}

	void sortTransparentByDistance()
	{
		std::sort(transparentItems.begin(), transparentItems.end(),
			[](const RenderItem& a, const RenderItem& b)
			{
				return a.sortKey > b.sortKey;
			});
	}
};

} // namespace kEngine
