#pragma once

#include "../common.h"
#include "entity.h"
#include "components.h"
#include <optional>

namespace kEngine
{

template<typename T>
class ComponentStorage
{
public:
	T& add(EntityID id)
	{
		m_data[id] = T{};
		return m_data[id];
	}

	T& get(EntityID id)
	{
		return m_data.at(id);
	}

	const T& get(EntityID id) const
	{
		return m_data.at(id);
	}

	bool has(EntityID id) const
	{
		return m_data.find(id) != m_data.end();
	}

	void remove(EntityID id)
	{
		m_data.erase(id);
	}

	void clear()
	{
		m_data.clear();
	}

	auto begin() { return m_data.begin(); }
	auto end() { return m_data.end(); }
	auto begin() const { return m_data.begin(); }
	auto end() const { return m_data.end(); }

private:
	std::unordered_map<EntityID, T> m_data;
};

class Scene
{
public:
	Scene() = default;
	~Scene() = default;

	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	// Entity management
	Entity createEntity(const std::string& name = "Entity");
	void destroyEntity(Entity entity);
	Entity findEntity(const std::string& name) const;

	const std::string& getEntityName(EntityID id) const
	{
		static std::string empty;
		auto it = m_names.find(id);
		return it != m_names.end() ? it->second : empty;
	}

	void setEntityName(EntityID id, const std::string& name)
	{
		m_names[id] = name;
	}

	// Camera
	void setMainCamera(Entity camera) { m_mainCamera = camera.id(); }
	Entity getMainCamera();

	// Component access
	template<typename T>
	T& addComponent(EntityID id)
	{
		return getStorage<T>().add(id);
	}

	template<typename T>
	T& getComponent(EntityID id)
	{
		return getStorage<T>().get(id);
	}

	template<typename T>
	const T& getComponent(EntityID id) const
	{
		return getStorage<T>().get(id);
	}

	template<typename T>
	bool hasComponent(EntityID id) const
	{
		return getStorage<T>().has(id);
	}

	template<typename T>
	void removeComponent(EntityID id)
	{
		getStorage<T>().remove(id);
	}

	// Iteration
	template<typename T>
	void each(std::function<void(Entity, T&)> fn)
	{
		for (auto& [id, comp] : getStorage<T>())
		{
			fn(Entity(id, this), comp);
		}
	}

	template<typename T1, typename T2>
	void each(std::function<void(Entity, T1&, T2&)> fn)
	{
		for (auto& [id, comp1] : getStorage<T1>())
		{
			if (hasComponent<T2>(id))
			{
				fn(Entity(id, this), comp1, getComponent<T2>(id));
			}
		}
	}

	void clear();

private:
	template<typename T> ComponentStorage<T>& getStorage();
	template<typename T> const ComponentStorage<T>& getStorage() const;

private:
	EntityID                                  m_nextId = 1;
	std::unordered_map<EntityID, bool>        m_entities;
	std::unordered_map<EntityID, std::string> m_names;
	EntityID                                  m_mainCamera = InvalidEntity;

	ComponentStorage<Transform>  m_transforms;
	ComponentStorage<Camera>     m_cameras;
	ComponentStorage<Renderable> m_renderables;
	ComponentStorage<Light>      m_lights;
};

// Entity template implementations
inline const std::string& Entity::name() const
{
	return m_scene->getEntityName(m_id);
}

inline void Entity::setName(const std::string& name)
{
	m_scene->setEntityName(m_id, name);
}

template<typename T>
T& Entity::add()
{
	return m_scene->addComponent<T>(m_id);
}

template<typename T>
T& Entity::get()
{
	return m_scene->getComponent<T>(m_id);
}

template<typename T>
const T& Entity::get() const
{
	return m_scene->getComponent<T>(m_id);
}

template<typename T>
bool Entity::has() const
{
	return m_scene->hasComponent<T>(m_id);
}

template<typename T>
void Entity::remove()
{
	m_scene->removeComponent<T>(m_id);
}

} // namespace kEngine
