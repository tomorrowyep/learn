#pragma once

#include "../common.h"
#include "mesh.h"
#include "texture.h"
#include "material.h"
#include "core/context/kcorecontext.h"
#include "core/descriptor/kdescriptorpool.h"
#include "core/descriptor/kdescriptorsetlayout.h"
#include "core/commands/kcommandpool.h"

namespace kEngine
{

class ResourceManager
{
public:
	ResourceManager() = default;
	~ResourceManager() = default;

	void init(KCoreContext* ctx);
	void destroy();

	// Mesh
	uint32_t loadMesh(const std::string& path);
	uint32_t createMesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices);
	Mesh* getMesh(uint32_t id);

	// Texture
	uint32_t loadTexture(const std::string& path);
	uint32_t createTexture(uint32_t width, uint32_t height, const void* data);
	Texture* getTexture(uint32_t id);
	uint32_t getDefaultWhiteTexture();

	// Material
	uint32_t createMaterial();
	uint32_t createMaterial(const MaterialData& data, uint32_t albedoTexture = 0);
	Material* getMaterial(uint32_t id);
	uint32_t getDefaultMaterial();

	VkDescriptorSetLayout getMaterialDescriptorSetLayout() const
	{
		return m_materialDescriptorSetLayout.GetHandle();
	}

private:
	bool uploadMesh(Mesh& mesh);
	bool uploadTexture(Texture& texture, const void* pixels);
	bool uploadMaterial(Material& material);
	void createMaterialDescriptorResources();

private:
	KCoreContext* m_ctx = nullptr;
	KCommandPool  m_commandPool;

	uint32_t m_nextMeshID = 1;
	uint32_t m_nextTextureID = 1;
	uint32_t m_nextMaterialID = 1;

	std::unordered_map<uint32_t, Scope<Mesh>>     m_meshes;
	std::unordered_map<uint32_t, Scope<Texture>>  m_textures;
	std::unordered_map<uint32_t, Scope<Material>> m_materials;

	std::unordered_map<std::string, uint32_t> m_meshCache;
	std::unordered_map<std::string, uint32_t> m_textureCache;

	uint32_t m_defaultWhiteTexture = 0;
	uint32_t m_defaultMaterial = 0;

	KDescriptorPool      m_materialDescriptorPool;
	KDescriptorSetLayout m_materialDescriptorSetLayout;
};

} // namespace kEngine
