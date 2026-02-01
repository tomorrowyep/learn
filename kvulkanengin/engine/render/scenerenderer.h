#pragma once

#include "../common.h"
#include "../scene/scene.h"
#include "renderdata.h"

namespace kEngine
{

class SceneRenderer
{
public:
	static void extract(const Scene& scene, RenderScene& renderScene, float aspect)
	{
		renderScene.clear();

		// Extract camera data
		Entity cameraEntity = const_cast<Scene&>(scene).getMainCamera();
		if (cameraEntity.valid() && cameraEntity.has<Camera>())
		{
			const auto& transform = cameraEntity.get<Transform>();
			const auto& camera = cameraEntity.get<Camera>();

			renderScene.cameraData.view = camera.getView(transform);
			renderScene.cameraData.projection = camera.getProjection(aspect);
			renderScene.cameraData.viewProjection =
				renderScene.cameraData.projection * renderScene.cameraData.view;
			renderScene.cameraData.position = glm::vec4(transform.position, 1.0f);
			renderScene.hasCamera = true;
		}

		// Extract renderable objects
		const_cast<Scene&>(scene).each<Renderable>([&](Entity entity, Renderable& renderable)
		{
			if (!renderable.visible)
				return;

			const auto& transform = entity.get<Transform>();

			RenderItem item;
			item.meshID = renderable.meshID;
			item.materialID = renderable.materialID;
			item.transform = transform.getMatrix();

			if (renderScene.hasCamera)
			{
				glm::vec3 camPos = glm::vec3(renderScene.cameraData.position);
				item.sortKey = glm::length(transform.position - camPos);
			}
			else
			{
				item.sortKey = 0.0f;
			}

			renderScene.opaqueItems.push_back(item);
		});

		renderScene.sortOpaqueByDistance();
		renderScene.sortTransparentByDistance();
	}
};

} // namespace kEngine
