#pragma once

#include "../common.h"
#include "core/objects/kbuffer.h"
#include "core/descriptor/kdescriptorset.h"

namespace kEngine
{

struct alignas(16) MaterialData
{
	glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
	float     metallic = 0.0f;
	float     roughness = 0.5f;
	float     ao = 1.0f;
	float     padding = 0.0f;
};

class Material
{
public:
	Material() = default;
	~Material() = default;

	Material(const Material&) = delete;
	Material& operator=(const Material&) = delete;

	Material(Material&&) = default;
	Material& operator=(Material&&) = default;

	MaterialData   data;

	uint32_t       albedoTexture = 0;
	uint32_t       normalTexture = 0;
	uint32_t       metallicTexture = 0;
	uint32_t       roughnessTexture = 0;

	KBuffer        uniformBuffer;
	KDescriptorSet descriptorSet;

	bool isUploaded() const { return m_uploaded; }
	void setUploaded(bool v) { m_uploaded = v; }

private:
	bool m_uploaded = false;
};

} // namespace kEngine
