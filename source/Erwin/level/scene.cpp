#include "level/scene.h"
#include "editor/font_awesome.h"
#include "editor/editor_components.h"
#include "entity/reflection.h"
#include "debug/logger.h"

#include <queue>
#include <tuple>

namespace erwin
{

EntityID Scene::selected_entity;
EntityID Scene::directional_light;
std::vector<EntityID> Scene::entities;
FreeflyController Scene::camera_controller;
entt::registry Scene::registry;

static std::queue<std::tuple<erwin::EntityID, uint32_t>> s_removed_components;
static std::queue<erwin::EntityID> s_removed_entities;

void Scene::init()
{
	camera_controller.init(1280.f/1024.f, 60, 0.1f, 100.f);
	selected_entity = k_invalid_entity_id;
	directional_light = k_invalid_entity_id;
}

void Scene::shutdown()
{

}

void Scene::add_entity(EntityID entity, const std::string& name, const char* _icon)
{
	ComponentEditorDescription desc = {name, (_icon) ? _icon : ICON_FA_CUBE, ""};
	registry.assign<ComponentEditorDescription>(entity, desc);

	entities.push_back(entity);

	DLOG("editor",1) << "[Scene] Added entity: " << name << std::endl;
}

void Scene::select(EntityID entity)
{
	selected_entity = entity;
}

void Scene::drop_selection()
{
	selected_entity = k_invalid_entity_id;
}

void Scene::mark_for_removal(erwin::EntityID entity, uint32_t reflected_component)
{
	s_removed_components.push({entity, reflected_component});
}

void Scene::mark_for_removal(erwin::EntityID entity)
{
	s_removed_entities.push(entity);
}

void Scene::cleanup()
{
	// Removed marked components
	while(!s_removed_components.empty())
	{
		auto&& [entity, reflected_component] = s_removed_components.front();
		invoke(W_METAFUNC_REMOVE_COMPONENT, reflected_component, Scene::registry, entity);
		s_removed_components.pop();
	}

	// Removed marked entities and all their components
	while(!s_removed_entities.empty())
	{
		auto entity = s_removed_entities.front();
		registry.destroy(entity);
		s_removed_entities.pop();
	}
}


} // namespace erwin