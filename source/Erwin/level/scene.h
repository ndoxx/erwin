#pragma once

#include <map>
#include <queue>

#include "asset/environment.h"
#include "entity/reflection.h"
#include "filesystem/file_path.h"

namespace erwin
{

struct ComponentHierarchy;

class Scene
{
public:
    using SceneVisitor = std::function<void(Scene&)>;

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
    // Setup a callback to run during deserialization to inject additional entities / components in this scene
    inline void set_injector_callback(SceneVisitor injector) { inject_ = injector; }
    // Setup a callback to run after deserialization is complete
    inline void set_finisher_callback(SceneVisitor finisher) { finish_ = finisher; }

    // Create an empty entity
    EntityID create_entity();
    // Create an entity and assign it a description component
    EntityID create_entity(const std::string& name, const char* icon = nullptr);
    // Check if scene has this entity
    inline bool has_entity(EntityID e) const { return registry.valid(e); }
    // Add a component to an entity
    template <typename ComponentT, typename... ArgsT> inline decltype(auto) add_component(EntityID e, ArgsT&&... args)
    {
        return registry.emplace<ComponentT>(e, std::forward<ArgsT>(args)...);
    }
    // Add a component to an entity or replace it if component already exists
    template <typename ComponentT, typename... ArgsT>
    inline decltype(auto) try_add_component(EntityID e, ArgsT&&... args)
    {
        return registry.emplace_or_replace<ComponentT>(e, std::forward<ArgsT>(args)...);
    }
    // Get a mutable ref to an entity component
    template <typename ComponentT> inline decltype(auto) get_component(EntityID e)
    {
        return registry.get<ComponentT>(e);
    }
    // Get a const ref to an entity component
    template <typename ComponentT> inline decltype(auto) get_component(EntityID e) const
    {
        return registry.get<ComponentT>(e);
    }
    // Clear all components of a given type
    template <typename... ComponentsT> inline void clear() { registry.clear<ComponentsT...>(); }
    // Get a component view
    template <typename... ComponentsT, typename... ExcludeT>
    inline decltype(auto) view(entt::exclude_t<ExcludeT...> exclude = {})
    {
        return registry.view<ComponentsT...>(exclude);
    }
    // Get a const component view
    template <typename... ComponentsT, typename... ExcludeT>
    inline decltype(auto) view(entt::exclude_t<ExcludeT...> exclude = {}) const
    {
        return registry.view<ComponentsT...>(exclude);
    }
    // Check if a given entity has specified components
    template <typename... ComponentsT> inline bool has_component(EntityID e) const
    {
        return registry.has<ComponentsT...>(e);
    }
    template <typename ComponentT> auto on_construct() { return registry.on_construct<ComponentT>(); }

	// For each component in an entity, invoke a function or any callable type taking
	// the component type ID and an opaque void* pointer to the component's instance as parameters
	// Note that func must be of an invocable type
	template <typename FuncType, typename = traits::IsInvocable<FuncType>>
	inline void visit_entity(entt::entity e, FuncType&& func)
	{
		registry.visit(e, [this,&func,e](uint64_t type_id)
		{
			uint32_t reflected_type = reflect(type_id);
			if(reflected_type==0)
				return;
			auto any = invoke(W_METAFUNC_GET_COMPONENT, reflected_type, registry, e);
			if(any)
				func(reflected_type, any.data());
		});
	}

	// For each component in an entity, invoke a meta-function by name.
	// The meta-function should exist in the meta-object associated to each component
	// this function call is susceptible to come upon.
	inline void visit_entity(entt::entity e, uint32_t meta_func)
	{
		registry.visit(e, [this,meta_func,e](uint64_t type_id)
		{
			uint32_t reflected_type = reflect(type_id);
			if(reflected_type==0)
				return;
			auto any = invoke(W_METAFUNC_GET_COMPONENT, reflected_type, registry, e);
			if(any)
				invoke(meta_func, reflected_type, any.data());
		});
	}

    // Avoid using these
    inline entt::registry& get_registry() { return registry; }
    inline const entt::registry& get_registry() const { return registry; }

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

    //* Hierarchy stuff
	// To be used with traversal algorithms. Return true to stop exploration in current branch.
	using NodeVisitor = std::function<bool(EntityID, const ComponentHierarchy&, size_t /*relative_depth*/)>;

	// Attach a subtree whose root is 'child' as a child of 'parent'
	void attach(EntityID parent, EntityID child);
	// Detach a subtree whose root is 'node' from its parent
	void detach(EntityID node);
	// Sort ComponentHierarchy pool such that parents are always visited before their children
	void sort_hierarchy();
	// Traverse hierarchy using a depth first algorithm and visit each node, till the visitor returns true
	void depth_first(EntityID node, NodeVisitor visit);
	// Depth-first traversal with the constraint that first children are always visited first (slower)
	void depth_first_ordered(EntityID node, NodeVisitor visit);
	// Traverse hierarchy using a breadth first algorithm and visit each node, till the visitor returns true
	void breadth_first(EntityID node, NodeVisitor visit);
	// Check if subtree of root 'root' contains the node 'node'
	bool subtree_contains(EntityID root, EntityID node); 
	// Check if node 'node' is a direct child of node 'parent'
	bool is_child(EntityID parent, EntityID node);
	// Check if two nodes are siblings
	bool is_sibling(EntityID first, EntityID second);


private:
    entt::registry registry;
    std::queue<std::tuple<EntityID, uint32_t>> removed_components_;
    std::queue<EntityID> removed_entities_;
    std::map<hash_t, EntityID> named_entities_;
    Environment environment_;
    SceneVisitor inject_ = [](auto&) {};
    SceneVisitor finish_ = [](auto&) {};
    size_t asset_registry_;
    bool loaded_ = false;

    FilePath scene_file_path_;
};

} // namespace erwin