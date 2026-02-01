#pragma once

#include "../common.h"
#include "core/objects/kimage.h"
#include "core/objects/kimageview.h"
#include "core/objects/ksampler.h"

namespace kEngine
{

class Texture
{
public:
	Texture() = default;
	~Texture() = default;

	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

	Texture(Texture&&) = default;
	Texture& operator=(Texture&&) = default;

	uint32_t   width = 0;
	uint32_t   height = 0;
	uint32_t   channels = 0;

	KImage     image;
	KImageView imageView;
	KSampler   sampler;

	bool isUploaded() const { return m_uploaded; }
	void setUploaded(bool v) { m_uploaded = v; }

private:
	bool m_uploaded = false;
};

} // namespace kEngine
