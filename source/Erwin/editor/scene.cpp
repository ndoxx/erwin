#include "scene.h"
#include "font_awesome.h"
#include "debug/logger.h"
#include "entity/entity_manager.h"

using namespace erwin;

namespace editor
{

EntityID Scene::selected_entity;
EntityID Scene::directional_light;
std::vector<EntityID> Scene::entities;
FreeflyController Scene::camera_controller;

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
	auto& ent = ECS::get_entity(entity);
	ent.set_name(name);
	if(_icon != nullptr)
		ent.set_icon(_icon);
	else
		ent.set_icon(ICON_FA_CUBE);

	entities.push_back(entity);

	DLOG("editor",1) << "[Scene] Added entity: " << name << " ID: " << entity << std::endl;
}

void Scene::select(EntityID entity)
{
	selected_entity = entity;
}

} // namespace editor