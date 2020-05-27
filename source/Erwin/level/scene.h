#pragma once

#include <filesystem>
#include <queue>

#include "entity/reflection.h"
#include "render/handles.h"

#include "level/scene_manager.h"

namespace fs = std::filesystem;

namespace erwin
{

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

	EntityID selected_entity = k_invalid_entity_id;
	EntityID directional_light = k_invalid_entity_id;
	EntityID camera = k_invalid_entity_id;
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