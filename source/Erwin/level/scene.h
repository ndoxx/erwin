#pragma once

#include <filesystem>
#include <queue>

#include "entity/reflection.h"
#include "render/handles.h"

#include "level/scene_manager.h"

namespace fs = std::filesystem;

namespace erwin
{

// Deprec
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
	static EntityID camera;
	static std::vector<EntityID> entities;

	static struct Environment
	{
	    CubemapHandle environment_map;
	    CubemapHandle diffuse_irradiance_map;
	    CubemapHandle prefiltered_env_map;
	} environment;

	static entt::registry registry;
};

class EdScene: public AbstractScene
{
public:
    virtual ~EdScene() override = default;

    // Load all needed data in graphics memory
    virtual void load() override;
    // Unload all graphics resources
    virtual void unload() override;
    // Cleanup all dead components and entities
    virtual void cleanup() override;


	void load_hdr_environment(const fs::path& hdr_file);

	void add_entity(EntityID entity, const std::string& name, const char* icon = nullptr);
	void select(EntityID entity);
	void drop_selection();

	void mark_for_removal(EntityID entity, uint32_t reflected_component);
	void mark_for_removal(EntityID entity);

	EntityID selected_entity;
	EntityID directional_light;
	EntityID camera;
	std::vector<EntityID> entities;

	struct Environment
	{
	    CubemapHandle environment_map;
	    CubemapHandle diffuse_irradiance_map;
	    CubemapHandle prefiltered_env_map;
	} environment;

	entt::registry registry;

private:
	std::queue<std::tuple<EntityID, uint32_t>> removed_components_;
	std::queue<EntityID> removed_entities_;
};

} // namespace erwin