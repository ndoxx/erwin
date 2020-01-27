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
	Scene() = default;

	void init();
	void shutdown();

	void add_entity(EntityID entity, const std::string& name, const char* icon = nullptr);
	void select(EntityID entity);

	inline EntityID get_selected_entity() const { return entities[selected_entity_idx].id; }

	uint32_t selected_entity_idx;
	EntityID directional_light;
	std::vector<EntityDescriptor> entities;
	std::map<EntityID, uint32_t> entity_index_lookup;

	FreeflyController camera_controller;
	PostProcessingData post_processing;
};

} // namespace erwin