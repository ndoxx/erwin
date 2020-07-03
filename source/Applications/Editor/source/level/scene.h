#pragma once

#include <queue>
#include <map>

#include "entity/reflection.h"
#include "asset/environment.h"
#include "filesystem/file_path.h"
#include "level/scene_manager.h"


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
	Scene();
    virtual ~Scene() override = default;

    // Cleanup all dead components and entities
    virtual void cleanup() override;

    void serialize_xml(const erwin::FilePath& file_path);
    void deserialize_xml(const erwin::FilePath& file_path);

	void load_hdr_environment(const erwin::FilePath& hdr_file);

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

	inline size_t get_asset_registry() const { return asset_registry_; }

	void set_named(erwin::EntityID ent, erwin::hash_t hname);
	inline erwin::EntityID get_named(erwin::hash_t hname) const { return named_entities_.at(hname); }
	inline const erwin::Environment& get_environment() const { return environment_; }

	entt::registry registry;

private:
	std::queue<std::tuple<erwin::EntityID, uint32_t>> removed_components_;
	std::queue<erwin::EntityID> removed_entities_;
	std::map<erwin::hash_t, erwin::EntityID> named_entities_;
	erwin::Environment environment_;
	size_t asset_registry_;
};

} // namespace editor