#pragma once

#include "../common.h"
#include "components.h"

namespace kEngine
{

class Scene;

class Entity
{
public:
	Entity() = default;
	Entity(EntityID id, Scene* scene) : m_id(id), m_scene(scene) {}

	EntityID id() const { return m_id; }
	bool valid() const { return m_id != InvalidEntity && m_scene != nullptr; }
	operator bool() const { return valid(); }
	operator EntityID() const { return m_id; }

	const std::string& name() const;
	void setName(const std::string& name);

	template<typename T> T& add();
	template<typename T> T& get();
	template<typename T> const T& get() const;
	template<typename T> bool has() const;
	template<typename T> void remove();

	bool operator==(const Entity& other) const { return m_id == other.m_id; }
	bool operator!=(const Entity& other) const { return m_id != other.m_id; }

private:
	EntityID m_id = InvalidEntity;
	Scene*   m_scene = nullptr;
};

} // namespace kEngine

namespace std
{
	template<>
	struct hash<kEngine::Entity>
	{
		size_t operator()(const kEngine::Entity& e) const
		{
			return hash<kEngine::EntityID>()(e.id());
		}
	};
}
