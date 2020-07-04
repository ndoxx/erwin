#pragma once

#include <queue>
#include <map>

#include "entity/reflection.h"
#include "asset/environment.h"
#include "filesystem/file_path.h"

namespace erwin
{

class Scene
{
public:
	using SceneInjector = std::function<void(Scene&)>;

	Scene();
    ~Scene() = default;

    // Load a scene from a .scn file (XML format)
    void load_xml(const FilePath& file_path);
    // Save a scene to a .scn file (XML format)
    void save_xml(const FilePath& file_path);
    // Save scene to the file it was loaded from (if any)
    void save();
    // Clear entity and asset registry 
    void unload();
    // Get location of current scene file
    inline const FilePath& get_file_location() const { return scene_file_path_; } 
    // Check if scene is loaded
    inline bool is_loaded() const { return loaded_; }
    // Setup a callback run during deserialization to inject additional entities / components in this scene
	inline void set_injector(SceneInjector injector) { inject_ = injector; }

    // Create an entity and assign it a description component
	EntityID create_entity(const std::string& name, const char* icon = nullptr);
	// Passed component will be removed next frame
	void mark_for_removal(EntityID entity, uint32_t reflected_component);
	// Passed entity and all its components will be removed next frame
	void mark_for_removal(EntityID entity);
    // Cleanup all dead components and entities
    void cleanup();
    // Make passed entity a named entity that can be queried by name
	void set_named(EntityID ent, hash_t hname);
	// Get a named entity by name
	inline EntityID get_named(hash_t hname) const { return named_entities_.at(hname); }
	// Check if a named entity is registered to this name
	inline bool has_named(hash_t hname) const { return (named_entities_.find(hname) != named_entities_.end()); }

    // Load an IBL environment from an equirectangular HDR image file
	void load_hdr_environment(const FilePath& hdr_file);
	// Get environment structure
	inline const Environment& get_environment() const { return environment_; }
	// Get handle to the asset registry used by this scene
	inline size_t get_asset_registry() const { return asset_registry_; }

	entt::registry registry;

private:
	std::queue<std::tuple<EntityID, uint32_t>> removed_components_;
	std::queue<EntityID> removed_entities_;
	std::map<hash_t, EntityID> named_entities_;
	Environment environment_;
	SceneInjector inject_ = [](auto&){};
	size_t asset_registry_;
    bool loaded_ = false;

    FilePath scene_file_path_;
};

} // namespace erwin