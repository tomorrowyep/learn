#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace kEngine
{

// Type aliases
using EntityID = uint32_t;
constexpr EntityID InvalidEntity = 0;

// Forward declarations
class Engine;
class Scene;
class Entity;
class RenderContext;
class RenderScene;
class ResourceManager;

// Smart pointer aliases
template<typename T>
using Ref = std::shared_ptr<T>;

template<typename T>
using Scope = std::unique_ptr<T>;

template<typename T, typename... Args>
constexpr Ref<T> createRef(Args&&... args)
{
	return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
constexpr Scope<T> createScope(Args&&... args)
{
	return std::make_unique<T>(std::forward<Args>(args)...);
}

// Engine configuration
struct EngineConfig
{
	uint32_t    width = 1280;
	uint32_t    height = 720;
	std::string title = "KVulkan Engine";
	bool        vsync = true;
	bool        validation = true;
};

} // namespace kEngine
