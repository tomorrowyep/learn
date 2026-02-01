#include "resourcemanager.h"
#include "core/commands/kcommandbuffer.h"
#include "core/commands/kcommandpool.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>

namespace kEngine
{

void ResourceManager::init(KCoreContext* ctx)
{
	m_ctx = ctx;

	VkDevice device = m_ctx->GetDevice().GetHandle();
	uint32_t queueFamily = m_ctx->GetDevice().GetQueueFamilies()._graphicsFamily;

	m_commandPool.Init(device, queueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	createMaterialDescriptorResources();
}

void ResourceManager::destroy()
{
	if (!m_ctx)
		return;

	for (auto& [id, mesh] : m_meshes)
	{
		if (mesh->isUploaded())
		{
			mesh->vertexBuffer.Destroy();
			mesh->indexBuffer.Destroy();
		}
	}

	for (auto& [id, texture] : m_textures)
	{
		if (texture->isUploaded())
		{
			texture->sampler.Destroy();
			texture->imageView.Destroy();
			texture->image.Destroy();
		}
	}

	for (auto& [id, material] : m_materials)
	{
		if (material->isUploaded())
		{
			material->uniformBuffer.Destroy();
		}
	}

	m_materialDescriptorPool.Destroy();
	m_materialDescriptorSetLayout.Destroy();
	m_commandPool.Destroy();

	m_meshes.clear();
	m_textures.clear();
	m_materials.clear();
	m_meshCache.clear();
	m_textureCache.clear();
	m_ctx = nullptr;
}

void ResourceManager::createMaterialDescriptorResources()
{
	VkDevice device = m_ctx->GetDevice().GetHandle();

	std::vector<VkDescriptorSetLayoutBinding> bindings =
	{
		{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
		{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
	};
	m_materialDescriptorSetLayout.Init(device, bindings);

	std::vector<VkDescriptorPoolSize> poolSizes =
	{
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 256},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256}
	};
	m_materialDescriptorPool.Init(device, poolSizes, 256);
}

uint32_t ResourceManager::loadMesh(const std::string& path)
{
	auto it = m_meshCache.find(path);
	if (it != m_meshCache.end())
		return it->second;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str()))
	{
		std::cerr << "[ResourceManager] Failed to load mesh: " << path << std::endl;
		return 0;
	}

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::unordered_map<Vertex, uint32_t> uniqueVertices;

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};
			vertex.position =
			{
				attrib.vertices[3 * index.vertex_index],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			if (index.normal_index >= 0)
			{
				vertex.normal =
				{
					attrib.normals[3 * index.normal_index],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};
			}

			if (index.texcoord_index >= 0)
			{
				vertex.texCoord =
				{
					attrib.texcoords[2 * index.texcoord_index],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};
			}

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			indices.push_back(uniqueVertices[vertex]);
		}
	}

	uint32_t id = createMesh(std::move(vertices), std::move(indices));
	if (id > 0)
		m_meshCache[path] = id;

	std::cout << "[ResourceManager] Loaded mesh: " << path << " (ID=" << id << ")" << std::endl;
	return id;
}

uint32_t ResourceManager::createMesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices)
{
	auto mesh = createScope<Mesh>();
	mesh->vertices = std::move(vertices);
	mesh->indices = std::move(indices);

	if (!uploadMesh(*mesh))
		return 0;

	uint32_t id = m_nextMeshID++;
	m_meshes[id] = std::move(mesh);
	return id;
}

Mesh* ResourceManager::getMesh(uint32_t id)
{
	auto it = m_meshes.find(id);
	return it != m_meshes.end() ? it->second.get() : nullptr;
}

bool ResourceManager::uploadMesh(Mesh& mesh)
{
	if (!m_ctx)
		return false;

	VkDevice device = m_ctx->GetDevice().GetHandle();
	VkPhysicalDevice physDevice = m_ctx->GetPhysicalDevice().GetHandle();

	VkDeviceSize vertexSize = sizeof(Vertex) * mesh.vertices.size();
	mesh.vertexBuffer.Init(device, physDevice, vertexSize,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	memcpy(mesh.vertexBuffer.Map(), mesh.vertices.data(), vertexSize);
	mesh.vertexBuffer.Unmap();

	VkDeviceSize indexSize = sizeof(uint32_t) * mesh.indices.size();
	mesh.indexBuffer.Init(device, physDevice, indexSize,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	memcpy(mesh.indexBuffer.Map(), mesh.indices.data(), indexSize);
	mesh.indexBuffer.Unmap();

	mesh.setUploaded(true);
	return true;
}

uint32_t ResourceManager::loadTexture(const std::string& path)
{
	auto it = m_textureCache.find(path);
	if (it != m_textureCache.end())
		return it->second;

	int width, height, channels;
	stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

	if (!pixels)
	{
		std::cerr << "[ResourceManager] Failed to load texture: " << path << std::endl;
		return 0;
	}

	uint32_t id = createTexture(width, height, pixels);
	stbi_image_free(pixels);

	if (id > 0)
	{
		m_textureCache[path] = id;
		std::cout << "[ResourceManager] Loaded texture: " << path << " (ID=" << id << ")" << std::endl;
	}

	return id;
}

uint32_t ResourceManager::createTexture(uint32_t width, uint32_t height, const void* data)
{
	auto texture = createScope<Texture>();
	texture->width = width;
	texture->height = height;
	texture->channels = 4;

	if (!uploadTexture(*texture, data))
		return 0;

	uint32_t id = m_nextTextureID++;
	m_textures[id] = std::move(texture);
	return id;
}

Texture* ResourceManager::getTexture(uint32_t id)
{
	auto it = m_textures.find(id);
	return it != m_textures.end() ? it->second.get() : nullptr;
}

uint32_t ResourceManager::getDefaultWhiteTexture()
{
	if (m_defaultWhiteTexture == 0)
	{
		uint32_t white = 0xFFFFFFFF;
		m_defaultWhiteTexture = createTexture(1, 1, &white);
	}
	return m_defaultWhiteTexture;
}

bool ResourceManager::uploadTexture(Texture& texture, const void* pixels)
{
	if (!m_ctx)
		return false;

	VkDevice device = m_ctx->GetDevice().GetHandle();
	VkPhysicalDevice physDevice = m_ctx->GetPhysicalDevice().GetHandle();
	VkQueue graphicsQueue = m_ctx->GetDevice().GetGraphicsQueue();
	VkDeviceSize imageSize = texture.width * texture.height * 4;

	// Create staging buffer
	KBuffer stagingBuffer;
	stagingBuffer.Init(device, physDevice, imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	memcpy(stagingBuffer.Map(), pixels, imageSize);
	stagingBuffer.Unmap();

	// Create image
	texture.image.Init(device, physDevice, texture.width, texture.height,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Record and submit commands
	KCommandBuffer cmdBuffer;
	cmdBuffer.Allocate(device, m_commandPool.GetHandle());
	VkCommandBuffer cmd = cmdBuffer.BeginSingleCommand();

	// Transition to transfer dst
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = texture.image.GetHandle();
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(cmd,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);

	// Copy buffer to image
	VkBufferImageCopy region{};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent = {texture.width, texture.height, 1};

	vkCmdCopyBufferToImage(cmd,
		stagingBuffer.GetHandle(),
		texture.image.GetHandle(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &region);

	// Transition to shader read
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &barrier);

	cmdBuffer.EndSingleCommand(cmd, graphicsQueue);
	cmdBuffer.Free(m_commandPool.GetHandle());
	stagingBuffer.Destroy();

	// Create image view
	texture.imageView.Init(device, texture.image.GetHandle(),
		VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);

	// Create sampler
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	texture.sampler.Init(device, samplerInfo);

	texture.setUploaded(true);
	return true;
}

uint32_t ResourceManager::createMaterial()
{
	return createMaterial(MaterialData{}, getDefaultWhiteTexture());
}

uint32_t ResourceManager::createMaterial(const MaterialData& data, uint32_t albedoTexture)
{
	auto material = createScope<Material>();
	material->data = data;
	material->albedoTexture = albedoTexture > 0 ? albedoTexture : getDefaultWhiteTexture();

	if (!uploadMaterial(*material))
		return 0;

	uint32_t id = m_nextMaterialID++;
	m_materials[id] = std::move(material);

	std::cout << "[ResourceManager] Created material (ID=" << id << ")" << std::endl;
	return id;
}

Material* ResourceManager::getMaterial(uint32_t id)
{
	auto it = m_materials.find(id);
	return it != m_materials.end() ? it->second.get() : nullptr;
}

uint32_t ResourceManager::getDefaultMaterial()
{
	if (m_defaultMaterial == 0)
		m_defaultMaterial = createMaterial();
	return m_defaultMaterial;
}

bool ResourceManager::uploadMaterial(Material& material)
{
	if (!m_ctx)
		return false;

	VkDevice device = m_ctx->GetDevice().GetHandle();
	VkPhysicalDevice physDevice = m_ctx->GetPhysicalDevice().GetHandle();

	// Create uniform buffer
	material.uniformBuffer.Init(device, physDevice, sizeof(MaterialData),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	memcpy(material.uniformBuffer.Map(), &material.data, sizeof(MaterialData));
	material.uniformBuffer.Unmap();

	// Allocate descriptor set
	if (!material.descriptorSet.Allocate(device,
		m_materialDescriptorPool.GetHandle(),
		m_materialDescriptorSetLayout.GetHandle()))
	{
		return false;
	}

	// Get texture
	Texture* albedoTex = getTexture(material.albedoTexture);
	if (!albedoTex || !albedoTex->isUploaded())
		albedoTex = getTexture(getDefaultWhiteTexture());

	// Update descriptor set
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = material.uniformBuffer.GetHandle();
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(MaterialData);

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = albedoTex->imageView.GetHandle();
	imageInfo.sampler = albedoTex->sampler.GetHandle();

	std::vector<VkWriteDescriptorSet> writes(2);

	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].dstSet = material.descriptorSet.GetHandle();
	writes[0].dstBinding = 0;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].descriptorCount = 1;
	writes[0].pBufferInfo = &bufferInfo;

	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].dstSet = material.descriptorSet.GetHandle();
	writes[1].dstBinding = 1;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writes[1].descriptorCount = 1;
	writes[1].pImageInfo = &imageInfo;

	material.descriptorSet.Update(writes);
	material.setUploaded(true);
	return true;
}

} // namespace kEngine
