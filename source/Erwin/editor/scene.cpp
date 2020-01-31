#include "scene.h"
#include "font_awesome.h"
#include "debug/logger.h"

namespace erwin
{

void Scene::init()
{
	camera_controller.init(1280.f/1024.f, 60, 0.1f, 100.f);
	selected_entity_idx = 0;
	directional_light = k_invalid_entity_id;
}

void Scene::shutdown()
{

}

void Scene::add_entity(EntityID entity, const std::string& name, const char* _icon)
{
	selected_entity_idx = entities.size();
	if(_icon == nullptr)
		entities.push_back(EntityDescriptor{entity, name, ICON_FA_CUBE});
	else
		entities.push_back(EntityDescriptor{entity, name, _icon});

	entity_index_lookup.insert(std::make_pair(entity, selected_entity_idx));

	DLOG("editor",1) << "[Scene] Added entity: " << name << " ID: " << entity << std::endl;
}

void Scene::select(EntityID entity)
{
	auto it = entity_index_lookup.find(entity);
	if(it != entity_index_lookup.end())
		selected_entity_idx = it->second;
}

} // namespace erwin