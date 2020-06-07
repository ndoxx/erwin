#pragma once

#include <filesystem>
#include <queue>

#include "entity/reflection.h"
#include "asset/environment.h"

#include "level/scene_manager.h"

namespace fs = std::filesystem;

namespace editor
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


	void load_hdr_environment(const fs::path& hdr_file);

	void add_entity(erwin::EntityID entity, const std::string& name, const char* icon = nullptr);
	inline erwin::EntityID create_entity(const std::string& name, const char* icon = nullptr)
	{
		auto ent = registry.create();
		add_entity(ent, name, icon);
		return ent;
	}

	void select(erwin::EntityID entity);
	void drop_selection();

	void mark_for_removal(erwin::EntityID entity, uint32_t reflected_component);
	void mark_for_removal(erwin::EntityID entity);

	erwin::EntityID selected_entity = erwin::k_invalid_entity_id;
	erwin::EntityID directional_light = erwin::k_invalid_entity_id;
	erwin::EntityID camera = erwin::k_invalid_entity_id;
	std::vector<erwin::EntityID> entities;

	erwin::Environment environment;

	entt::registry registry;

private:
	std::queue<std::tuple<erwin::EntityID, uint32_t>> removed_components_;
	std::queue<erwin::EntityID> removed_entities_;
};

} // namespace editor