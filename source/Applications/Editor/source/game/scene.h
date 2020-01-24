#pragma once

#include "erwin.h"
#include "game/freefly_camera_controller.h"

namespace game
{

struct EntityDescriptor
{
	erwin::EntityID id;
	std::string name;
	const char* icon;
};

class Scene
{
public:
	Scene();

	void add_entity(erwin::EntityID entity, const std::string& name, const char* icon = nullptr);

	uint32_t selected_entity_idx;
	erwin::EntityID directional_light;
	std::vector<EntityDescriptor> entities;

	FreeflyController camera_controller;
	erwin::PostProcessingData post_processing;
};

} // namespace editor