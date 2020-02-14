#pragma once

#include "input/freefly_camera_controller.h"
#include "entity/reflection.h"

namespace editor
{

class Scene
{
public:
	static void init();
	static void shutdown();

	static void add_entity(erwin::EntityID entity, const std::string& name, const char* icon = nullptr);
	static void select(erwin::EntityID entity);

	static erwin::EntityID selected_entity;
	static erwin::EntityID directional_light;
	static std::vector<erwin::EntityID> entities;
	static erwin::FreeflyController camera_controller;

	static entt::registry registry;
};

} // namespace editor