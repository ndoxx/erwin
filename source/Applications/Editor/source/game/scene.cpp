#include "game/scene.h"
#include "game/game_components.h"

using namespace erwin;

namespace game
{

Scene::Scene():
camera_controller(1280.f/1024.f, 60, 0.1f, 100.f)
{

}

void Scene::add_entity(erwin::EntityID entity)
{
	entities_.push_back(entity);
}


} // namespace editor