#pragma once

#include <map>
#include <queue>

#include "asset/environment.h"
#include "entity/reflection.h"
#include "script/script_engine.h"
#include "filesystem/file_path.h"

namespace erwin
{

struct ComponentHierarchy;

class Scene
{
public:
    using SceneVisitor = std::function<void(Scene&)>;
	// To be used with traversal algorithms. Return true to stop exploration in current branch.
	using NodeVisitor = std::function<bool(EntityID, const ComponentHierarchy&, size_t /*relative_depth*/)>;

    //* (DE)SERIALIZATION

    /**
     * @brief      Create a default scene.
     *             Also sets up tag component creation callbacks.
     */
    Scene();
    ~Scene() = default;

    /**
     * @brief      Loads a scene from an XML file.
     *
     * @param[in]  file_path  Path to a valid .scn XML format file.
     */
    void load_xml(const FilePath& file_path);
    
    /**
     * @brief      Save current scene to a .scn XML format file.
     *
     * @param[in]  file_path  The file path.
     */
    void save_xml(const FilePath& file_path);
    
    // Save scene to the file it was loaded from (if any)
    
    /**
     * @brief      Save current scene to last .scn file path. The scene must
     *             have been saved via a call to save_xml() first in order for
     *             this to work.
     */
    void save();

    /**
     * @brief      Clear entity and asset registries
     */
    void unload();
    
    /**
     * @brief      Determines if scene is runtime.
     *
     * @return     True if runtime, False otherwise.
     */
    inline bool is_runtime() const { return runtime_; }

    /**
     * @brief      Make a shallow copy of another scene and load it.
     *
     * @param[in]  other  The source scene.
     */
    void runtime_clone(const Scene& other);
    
    /**
     * @brief      Get location of current scene file.
     *
     * @return     The file location.
     */
    inline const FilePath& get_file_location() const { return scene_file_path_; }
    
    /**
     * @brief      Check if scene is loaded.
     *
     * @return     True if loaded, False otherwise.
     */
    inline bool is_loaded() const { return loaded_; }
    
    /**
     * @brief      Setup a callback to run during deserialization to inject additional entities / components in this scene.
     *
     * @param[in]  injector  The injector
     */
    inline void set_injector_callback(SceneVisitor injector) { inject_ = injector; }
    
    /**
     * @brief      Setup a callback to run after deserialization is complete.
     *
     * @param[in]  finisher  The finisher
     */
    inline void set_finisher_callback(SceneVisitor finisher) { finish_ = finisher; }

    /**
     * @brief      Load an IBL environment from an equirectangular HDR image file.
     *
     * @param[in]  hdr_file  Path to a valid HDR image file.
     */
    void load_hdr_environment(const FilePath& hdr_file);
    
    /**
     * @brief      Gets the environment.
     *
     * @return     The environment.
     */
    inline const Environment& get_environment() const { return environment_; }
    
    /**
     * @brief      Gets a handle to the asset registry in use by this scene.
     *
     * @return     Handle to the asset registry.
     */
    inline size_t get_asset_registry() const { return asset_registry_; }


    //* ENTITY MANIPULATION

    /**
     * @brief      Create an empty entity.
     *
     * @return     Handle to the newly created entity.
     */
    EntityID create_entity();
    
    // Create an entity and assign it a description component
    
    /**
     * @brief      Create an entity and assign it a description component.
     *
     * @param[in]  name  Name of the entity (for editor purposes)
     * @param[in]  icon  Icon associated to the entity (for editor purposes)
     *
     * @return     Handle to the newly created entity.
     */
    EntityID create_entity(const std::string& name, const char* icon = nullptr);
    
    /**
     * @brief      Check if scene has this entity.
     *
     * @param[in]  e     The entity id to test for.
     *
     * @return     True if exists, False otherwise.
     */
    inline bool has_entity(EntityID e) const { return registry.valid(e); }
    
    // Add a component to an entity
    
    /**
     * @brief      Add a component to an entity.
     *
     * @tparam     ComponentT  Type of component to add.
     * @tparam     ArgsT       Types of the constructor parameters.
     * @param[in]  e           Handle to the target entity.
     * @param[in]  args        Component constructor parameters to be forwarded.
     * 
     * @return     Reference to the newly created component.
     */
    template <typename ComponentT, typename... ArgsT> inline decltype(auto) add_component(EntityID e, ArgsT&&... args)
    {
        return registry.emplace<ComponentT>(e, std::forward<ArgsT>(args)...);
    }
    
    /**
     * @brief      Add a component to an entity or replace it if component already exists.
     *
     * @tparam     ComponentT  Type of component to add.
     * @tparam     ArgsT       Types of the constructor parameters.
     * @param[in]  e           Handle to the target entity.
     * @param[in]  args        Component constructor parameters to be forwarded.
     * 
     * @return     Reference to the newly created component.
     */
    template <typename ComponentT, typename... ArgsT>
    inline decltype(auto) try_add_component(EntityID e, ArgsT&&... args)
    {
        return registry.emplace_or_replace<ComponentT>(e, std::forward<ArgsT>(args)...);
    }
    
    /**
     * @brief      Get a mutable ref to an entity component.
     *
     * @tparam     ComponentT  Type of component to add.
     * @param[in]  e           Handle to the entity.
     * 
     * @return     Reference to the component.
     */
    template <typename ComponentT> inline decltype(auto) get_component(EntityID e)
    {
        return registry.get<ComponentT>(e);
    }

    /**
     * @brief      Get a const ref to an entity component.
     *
     * @tparam     ComponentT  Type of component to add.
     * @param[in]  e           Handle to the entity.
     * 
     * @return     Reference to the component.
     */
    template <typename ComponentT> inline decltype(auto) get_component(EntityID e) const
    {
        return registry.get<ComponentT>(e);
    }
    
    /**
     * @brief      Clear all components of given types.
     *
     * @tparam     ComponentsT  List of component types to clear.
     */
    template <typename... ComponentsT> inline void clear() { registry.clear<ComponentsT...>(); }
    
    /**
     * @brief      Get a component view
     *
     * @tparam     ComponentsT Component types an entity is required to have in order to be in this view.
     * @tparam     ExcludeT    Component types an entity is required NOT to have in order to be in this view.
     * @param[in]  exclude     The exclusion object.
     * 
     * @return     The view.
     */
    template <typename... ComponentsT, typename... ExcludeT>
    inline decltype(auto) view(entt::exclude_t<ExcludeT...> exclude = {})
    {
        return registry.view<ComponentsT...>(exclude);
    }

    /**
     * @brief      Get a const component view
     *
     * @tparam     ComponentsT Component types an entity is required to have in order to be in this view.
     * @tparam     ExcludeT    Component types an entity is required NOT to have in order to be in this view.
     * @param[in]  exclude     The exclusion object.
     * 
     * @return     The view.
     */
    template <typename... ComponentsT, typename... ExcludeT>
    inline decltype(auto) view(entt::exclude_t<ExcludeT...> exclude = {}) const
    {
        return registry.view<ComponentsT...>(exclude);
    }
    
    /**
     * @brief      Determines if an entity has specified components.
     *
     * @param[in]  e            The entity handle.
     *
     * @tparam     ComponentsT  List of component types the entity is required to have.
     *
     * @return     True if all components exist, False otherwise.
     */
    template <typename... ComponentsT> inline bool has_component(EntityID e) const
    {
        return registry.has<ComponentsT...>(e);
    }
    
    /**
     * @brief      Called on component construct.
     *
     * @tparam     ComponentT  Type of component that will trigger a call on construction.
     *
     * @return     Connectable object.
     */
    template <typename ComponentT> auto on_construct() { return registry.on_construct<ComponentT>(); }

	/**
	 * @brief      Invoke a function for each component of an entity.
	 * 
	 *             For each component in an entity, invoke a function or any callable
	 *             type taking the component type ID and an opaque void* pointer
	 *             to the component's instance as parameters Note that func must
	 *             be of an invocable type
	 *
	 * @param[in]  e          Target entity.
	 * @param      func       Function to be called on each component of the
	 *                        entity.
	 *
	 * @tparam     FuncType   Any callable type.
	 */
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

	/**
	 * @brief      Invoke a function for each component of an entity.
	 *
	 *             For each component in an entity, invoke a meta-function by
	 *             name. The meta-function should exist in the meta-object
	 *             associated to each component this function call is
	 *             susceptible to come upon.
	 *
	 * @param[in]  e          Target entity.
	 * @param      meta_func  Id of function to be called on each component of
	 *                        the entity.
	 */
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

    /**
     * @brief      Gets the entity registry.
     *
     *             Avoid using this function, it will soon disappear from the
     *             API.
     *
     * @return     The registry.
     */
    inline entt::registry& get_registry() { return registry; }

    /**
     * @brief      Gets the entity registry.
     *
     *             Avoid using this function, it will soon disappear from the
     *             API.
     *
     * @return     The registry.
     */
    inline const entt::registry& get_registry() const { return registry; }

    /**
     * @brief      Passed component will be removed next frame
     *
     * @param[in]  entity               Target entity
     * @param[in]  reflected_component  The reflected target component
     */
    void mark_for_removal(EntityID entity, uint32_t reflected_component);
    
    /**
     * @brief      Passed entity and all its components will be removed next frame
     *
     * @param[in]  entity  Target entity
     */
    void mark_for_removal(EntityID entity);
    
    /**
     * @brief      Cleanup all dead components and entities
     */
    void cleanup();
    
    /**
     * @brief      Make passed entity a named entity that can be queried by name
     *
     * @param[in]  ent    Target entity
     * @param[in]  hname  The entity's name as a hashed string
     */
    void set_named(EntityID ent, hash_t hname);
    
    /**
     * @brief      Get a named entity by name
     *
     * @param[in]  hname  The entity's name as a hashed string
     *
     * @return     Handle to the named entity.
     */
    inline EntityID get_named(hash_t hname) const { return named_entities_.at(hname); }
    
    /**
     * @brief      Check if a named entity is registered to this name.
     *
     * @param[in]  hname  The entity's name as a hashed string
     *
     * @return     True if named entity exists, False otherwise.
     */
    inline bool has_named(hash_t hname) const { return (named_entities_.find(hname) != named_entities_.end()); }


    //* Hierarchy stuff

	/**
	 * @brief      Attach a subtree whose root is 'child' as a child of 'parent'.
	 *
	 * @param[in]  parent  The parent.
	 * @param[in]  child   The child.
	 */
	void attach(EntityID parent, EntityID child);
	
	/**
	 * @brief      Detach a subtree whose root is 'node' from its parent
	 *
	 * @param[in]  node  The node
	 */
	void detach(EntityID node);
	
	/**
	 * @brief      Sort ComponentHierarchy pool such that parents are always visited before their children
	 */
	void sort_hierarchy();
	
	/**
	 * @brief      Traverse hierarchy using a depth first algorithm and visit each node, till the visitor returns true
	 *
	 * @param[in]  node   The first node.
	 * @param[in]  visit  Visitor function.
	 */
	void depth_first(EntityID node, NodeVisitor visit);
	
	/**
	 * @brief      Depth-first traversal with the constraint that first children are always visited first (slower)
	 *
	 * @param[in]  node   The first node.
	 * @param[in]  visit  Visitor function.
	 */
	void depth_first_ordered(EntityID node, NodeVisitor visit);
	
	/**
	 * @brief      Traverse hierarchy using a breadth first algorithm and visit each node, till the visitor returns true
	 *
	 * @param[in]  node   The first node.
	 * @param[in]  visit  Visitor function.
	 */
	void breadth_first(EntityID node, NodeVisitor visit);
	
	/**
	 * @brief      Check if subtree of root 'root' contains the node 'node'
	 *
	 * @param[in]  root  The root entity.
	 * @param[in]  node  The node to search for in the root's subtree.
	 *
	 * @return     True if node inside root's subtree, False otherwise.
	 */
	bool subtree_contains(EntityID root, EntityID node); 
	
	/**
	 * @brief      Check if node 'node' is a direct child of node 'parent'
	 *
	 * @param[in]  parent  The potential parent.
	 * @param[in]  node    The node to check.
	 *
	 * @return     True if child, False otherwise.
	 */
	bool is_child(EntityID parent, EntityID node);
	
	/**
	 * @brief      Check if two nodes are siblings.
	 *
	 * @param[in]  first   The first node.
	 * @param[in]  second  The second node.
	 *
	 * @return     True if the two nodes are siblings, False otherwise.
	 */
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
    script::VMHandle script_context_;
    bool loaded_ = false;
    bool runtime_ = false;

    FilePath scene_file_path_;
};

} // namespace erwin