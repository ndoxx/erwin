#include "level/scene.h"
#include "editor/font_awesome.h"
#include "editor/editor_components.h"
#include "debug/logger.h"

namespace erwin
{

EntityID Scene::selected_entity;
EntityID Scene::directional_light;
std::vector<EntityID> Scene::entities;
FreeflyController Scene::camera_controller;
entt::registry Scene::registry;

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


} // namespace erwin