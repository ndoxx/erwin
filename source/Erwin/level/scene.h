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
	Scene();
    ~Scene() = default;

    void load();
    void unload();
    inline bool is_loaded() const { return loaded_; }

    // Cleanup all dead components and entities
    void cleanup();

    void serialize_xml(const FilePath& file_path);
    void deserialize_xml(const FilePath& file_path);

	void load_hdr_environment(const FilePath& hdr_file);

	EntityID create_entity(const std::string& name, const char* icon = nullptr);

	void mark_for_removal(EntityID entity, uint32_t reflected_component);
	void mark_for_removal(EntityID entity);

	inline size_t get_asset_registry() const { return asset_registry_; }

	void set_named(EntityID ent, hash_t hname);
	inline EntityID get_named(hash_t hname) const { return named_entities_.at(hname); }
	inline bool has_named(hash_t hname) const { return (named_entities_.find(hname) != named_entities_.end()); }
	inline const Environment& get_environment() const { return environment_; }

	using SceneInjector = std::function<void(Scene&)>;
	inline void set_injector(SceneInjector injector) { inject_ = injector; }
	inline void set_assets_root(const fs::path& dirpath) { root_dir_ = dirpath; }

	entt::registry registry;

private:
	std::queue<std::tuple<EntityID, uint32_t>> removed_components_;
	std::queue<EntityID> removed_entities_;
	std::map<hash_t, EntityID> named_entities_;
	Environment environment_;
	size_t asset_registry_;
	SceneInjector inject_ = [](auto&){};
	fs::path root_dir_;
    bool loaded_ = false;
};

} // namespace erwin