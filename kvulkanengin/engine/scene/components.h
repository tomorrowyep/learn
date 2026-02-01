#pragma once

#include "../common.h"
#include <cmath>

namespace kEngine
{

// Transform component
struct Transform
{
	glm::vec3 position{0.0f};
	glm::vec3 rotation{0.0f};    // Euler angles (degrees)
	glm::vec3 scale{1.0f};

	glm::mat4 getMatrix() const
	{
		glm::mat4 m{1.0f};
		m = glm::translate(m, position);
		m = glm::rotate(m, glm::radians(rotation.y), glm::vec3{0, 1, 0});
		m = glm::rotate(m, glm::radians(rotation.x), glm::vec3{1, 0, 0});
		m = glm::rotate(m, glm::radians(rotation.z), glm::vec3{0, 0, 1});
		m = glm::scale(m, scale);
		return m;
	}

	glm::vec3 forward() const
	{
		float yaw = glm::radians(rotation.y);
		float pitch = glm::radians(rotation.x);
		return glm::normalize(glm::vec3{
			std::sin(yaw) * std::cos(pitch),
			-std::sin(pitch),
			-std::cos(yaw) * std::cos(pitch)
		});
	}

	glm::vec3 right() const
	{
		return glm::normalize(glm::cross(forward(), glm::vec3{0, 1, 0}));
	}

	glm::vec3 up() const
	{
		return glm::normalize(glm::cross(right(), forward()));
	}
};

// Camera component
struct Camera
{
	enum class Projection { Perspective, Orthographic };

	Projection projection = Projection::Perspective;
	float      fov = 60.0f;
	float      orthoSize = 5.0f;
	float      nearClip = 0.1f;
	float      farClip = 1000.0f;

	glm::mat4 getProjection(float aspect) const
	{
		glm::mat4 proj;
		if (projection == Projection::Perspective)
		{
			proj = glm::perspective(glm::radians(fov), aspect, nearClip, farClip);
		}
		else
		{
			float w = orthoSize * aspect;
			float h = orthoSize;
			proj = glm::ortho(-w, w, -h, h, nearClip, farClip);
		}
		proj[1][1] *= -1; // Vulkan Y-flip
		return proj;
	}

	glm::mat4 getView(const Transform& t) const
	{
		return glm::lookAt(t.position, t.position + t.forward(), glm::vec3{0, 1, 0});
	}
};

// Renderable component
struct Renderable
{
	uint32_t meshID = 0;
	uint32_t materialID = 0;
	bool     visible = true;
	bool     castShadow = true;
	bool     receiveShadow = true;
};

// Light component (reserved)
struct Light
{
	enum class Type { Directional, Point, Spot };

	Type      type = Type::Directional;
	glm::vec3 color{1.0f};
	float     intensity = 1.0f;
	float     range = 10.0f;
	float     spotAngle = 45.0f;
	float     spotSoftness = 0.5f;
};

} // namespace kEngine
