#include "scene.h"

namespace kEngine
{

// Component storage specializations
template<> ComponentStorage<Transform>& Scene::getStorage() { return m_transforms; }
template<> ComponentStorage<Camera>& Scene::getStorage() { return m_cameras; }
template<> ComponentStorage<Renderable>& Scene::getStorage() { return m_renderables; }
template<> ComponentStorage<Light>& Scene::getStorage() { return m_lights; }

template<> const ComponentStorage<Transform>& Scene::getStorage() const { return m_transforms; }
template<> const ComponentStorage<Camera>& Scene::getStorage() const { return m_cameras; }
template<> const ComponentStorage<Renderable>& Scene::getStorage() const { return m_renderables; }
template<> const ComponentStorage<Light>& Scene::getStorage() const { return m_lights; }

Entity Scene::createEntity(const std::string& name)
{
	EntityID id = m_nextId++;
	m_entities[id] = true;
	m_names[id] = name;

	addComponent<Transform>(id);

	return Entity(id, this);
}

void Scene::destroyEntity(Entity entity)
{
	EntityID id = entity.id();

	m_transforms.remove(id);
	m_cameras.remove(id);
	m_renderables.remove(id);
	m_lights.remove(id);

	m_entities.erase(id);
	m_names.erase(id);

	if (m_mainCamera == id)
	{
		m_mainCamera = InvalidEntity;
	}
}

Entity Scene::findEntity(const std::string& name) const
{
	for (const auto& [id, n] : m_names)
	{
		if (n == name)
		{
			return Entity(id, const_cast<Scene*>(this));
		}
	}
	return Entity();
}

Entity Scene::getMainCamera()
{
	if (m_mainCamera != InvalidEntity && m_entities.count(m_mainCamera))
	{
		return Entity(m_mainCamera, this);
	}
	return Entity();
}

void Scene::clear()
{
	m_transforms.clear();
	m_cameras.clear();
	m_renderables.clear();
	m_lights.clear();
	m_entities.clear();
	m_names.clear();
	m_mainCamera = InvalidEntity;
	m_nextId = 1;
}

} // namespace kEngine
