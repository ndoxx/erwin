#pragma once

#include "input/freefly_camera_controller.h"
#include "entity/entity_types.h"
#include "render/renderer_pp.h"

namespace erwin
{

struct EntityDescriptor
{
	EntityID id;
	std::string name;
	const char* icon;
};

class Scene
{
public:
	Scene();

	void add_entity(EntityID entity, const std::string& name, const char* icon = nullptr);

	inline EntityID get_selected_entity() const { return entities[selected_entity_idx].id; }

	uint32_t selected_entity_idx;
	EntityID directional_light;
	std::vector<EntityDescriptor> entities;

	FreeflyController camera_controller;
	PostProcessingData post_processing;
};

} // namespace erwin