#pragma once

#include "input/freefly_camera_controller.h"
#include "entity/entity_types.h"

namespace editor
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
	static void init();
	static void shutdown();

	static void add_entity(erwin::EntityID entity, const std::string& name, const char* icon = nullptr);
	static void select(erwin::EntityID entity);

	static inline erwin::EntityID get_selected_entity() { return entities[selected_entity_idx].id; }

	static uint32_t selected_entity_idx;
	static erwin::EntityID directional_light;
	static std::vector<EntityDescriptor> entities;
	static std::map<erwin::EntityID, uint32_t> entity_index_lookup;
	static erwin::FreeflyController camera_controller;
};

} // namespace editor