#include "scene.h"
#include "entity/entity_types.h"
#include "font_awesome.h"
#include "debug/logger.h"

namespace erwin
{

Scene::Scene():
camera_controller(1280.f/1024.f, 60, 0.1f, 100.f)
{
	selected_entity_idx = 0;
}

void Scene::add_entity(EntityID entity, const std::string& name, const char* _icon)
{
	selected_entity_idx = entities.size();
	if(_icon == nullptr)
		entities.push_back(EntityDescriptor{entity, name, ICON_FA_CUBE});
	else
		entities.push_back(EntityDescriptor{entity, name, _icon});

	DLOG("editor",1) << "[Scene] Added entity: " << name << " ID: " << entity << std::endl;
}


} // namespace erwin