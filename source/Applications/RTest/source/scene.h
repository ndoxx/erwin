#pragma once

#include <filesystem>
#include <queue>

#include "entity/reflection.h"

#include "level/scene_manager.h"
#include "asset_manager_exp.h"

namespace fs = std::filesystem;

namespace rtest
{

class Scene: public erwin::AbstractScene
{
protected:
    // Load all needed data in graphics memory
    virtual bool on_load() override;
    // Unload all graphics resources
    virtual void on_unload() override;
	
public:
    virtual ~Scene() override = default;

    // Cleanup all dead components and entities
    virtual void cleanup() override;

	void add_entity(erwin::EntityID entity);
	inline erwin::EntityID create_entity()
	{
		auto ent = registry.create();
		add_entity(ent);
		return ent;
	}

	erwin::EntityID camera = erwin::k_invalid_entity_id;
	erwin::EntityID directional_light = erwin::k_invalid_entity_id;
	std::vector<erwin::EntityID> entities;

	struct Environment
	{
	    erwin::CubemapHandle environment_map;
	    erwin::CubemapHandle diffuse_irradiance_map;
	    erwin::CubemapHandle prefiltered_env_map;
	} environment;

	entt::registry registry;
};

} // namespace rtest