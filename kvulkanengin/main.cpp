#include "engine/engine.h"

using namespace kEngine;

int main()
{
	// Create and initialize engine
	Engine engine;

	EngineConfig config;
	config.width = 1280;
	config.height = 720;
	config.title = "KVulkan Engine";
	config.vsync = true;
	config.validation = true;

	if (!engine.init(config))
		return -1;

	// Setup scene
	Scene& scene = engine.scene();
	ResourceManager& resources = engine.resources();

	// Create camera
	Entity cameraEntity = scene.createEntity("MainCamera");
	cameraEntity.get<Transform>().position = glm::vec3(0.0f, 0.0f, 3.0f);
	cameraEntity.add<Camera>().fov = 45.0f;
	scene.setMainCamera(cameraEntity);

	// Load resources
	uint32_t meshID = resources.loadMesh("assets/models/african_head.obj");
	uint32_t textureID = resources.loadTexture("assets/textures/african_head_diffuse.jpg");

	// Create material
	MaterialData matData;
	matData.baseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	matData.metallic = 0.0f;
	matData.roughness = 0.5f;
	matData.ao = 1.0f;
	uint32_t materialID = resources.createMaterial(matData, textureID);

	// Create model entity
	Entity modelEntity = scene.createEntity("Model");
	modelEntity.get<Transform>().scale = glm::vec3(1.0f, 1.0f, 1.0f);
	Renderable& renderable = modelEntity.add<Renderable>();
	renderable.meshID = meshID;
	renderable.materialID = materialID;

	// Run main loop
	engine.run([&](float dt)
	{
		modelEntity.get<Transform>().rotation.y += 30.0f * dt;
	});

	return 0;
}
