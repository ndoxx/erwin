#pragma once

#include <filesystem>

#include "input/freefly_camera_controller.h"
#include "entity/reflection.h"
#include "render/handles.h"

namespace fs = std::filesystem;

namespace erwin
{

class Scene
{
public:
	static void load_hdr_environment(const fs::path& hdr_file);

	static void add_entity(EntityID entity, const std::string& name, const char* icon = nullptr);
	static void select(EntityID entity);
	static void drop_selection();

	static void mark_for_removal(EntityID entity, uint32_t reflected_component);
	static void mark_for_removal(EntityID entity);

	static void cleanup();

	static EntityID selected_entity;
	static EntityID directional_light;
	static std::vector<EntityID> entities;
	static FreeflyController camera_controller;

	static struct Environment
	{
	    CubemapHandle environment_map;
	    CubemapHandle diffuse_irradiance_map;
	    CubemapHandle prefiltered_env_map;
	} environment;

	static entt::registry registry;
};

} // namespace erwin