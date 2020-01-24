#include "game/scene.h"
#include "game/game_components.h"
#include "font_awesome4.h"

using namespace erwin;

namespace game
{

Scene::Scene():
camera_controller(1280.f/1024.f, 60, 0.1f, 100.f)
{
	selected_entity_idx = 0;
}

void Scene::add_entity(erwin::EntityID entity, const std::string& name, const char* _icon)
{
	selected_entity_idx = entities.size();
	if(_icon == nullptr)
		entities.push_back(EntityDescriptor{entity, name, ICON_FA_CUBE});
	else
		entities.push_back(EntityDescriptor{entity, name, _icon});
}


} // namespace editor