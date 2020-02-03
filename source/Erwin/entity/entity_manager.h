#pragma once

/*
	Original design by Rez Bot: https://www.youtube.com/watch?v=5KugyHKsXLQ
*/

#include <vector>
#include <map>
#include <type_traits>

#include "EASTL/hash_map.h"
#include "EASTL/vector.h"
#include "entity/entity.h"
#include "entity/component_manager.h"
#include "entity/base_component_system.h"

namespace erwin
{

class Component;
class BaseComponentSystem;
class GameClock;
class W_API EntityManager
{
public:
	// Create a component system, only enabled for types inheritting from BaseComponentSystem
	template <typename SystemT, typename = typename std::enable_if<std::is_base_of<BaseComponentSystem, SystemT>::value, SystemT>::type>
	static SystemT* create_system()
	{
		SystemT* psys = new SystemT();
		systems_.push_back(psys);
		return psys;
	}

	// Create a component container and initialize component pool, only enabled for types inheritting from Component
	template <typename ComponentT, typename = typename std::enable_if<std::is_base_of<Component, ComponentT>::value, ComponentT>::type>
	static void create_component_manager(memory::HeapArea& area, size_t max_components)
	{
		auto* pmgr = new ComponentManager<ComponentT>(area, max_components);
		size_t index = components_.size();
		components_.push_back(pmgr);
		components_lookup_.emplace(ComponentT::ID, index);
	}

	// Update all systems
	static void update(const GameClock& clock);
	// For all systems that can render, do render
	static void render();
	// Create a new entity and return its ID
	static EntityID create_entity();
	// Register an entity's components into all relevant systems
	static void submit_entity(EntityID entity_id);
	// Remove an entity from existence, destroying all of its components
	static void destroy_entity(EntityID entity_id);
	// Display all component inspector GUI widgets for a given entity
	static void inspector_GUI(EntityID entity_id);

	// Create a new component for specified entity and optionally initialize it
	template <typename ComponentT>
	static ComponentT& create_component(EntityID entity_id, void* p_component_description=nullptr)
	{
		// Create and initialize component
		auto* pcmp = get_component_manager<ComponentT>()->create(entity_id);
		pcmp->set_pool_index(get_component_manager_index<ComponentT>());
		pcmp->set_parent_entity(entity_id);
		if(p_component_description)
			pcmp->init(p_component_description);

		// Register component in entity
		Entity& entity = get_entity(entity_id);
		entity.add_component(pcmp);

		return *pcmp;
	}

	// Get an entity by ID
	static inline Entity& get_entity(EntityID id)
	{
		auto it = entities_.find(id);
		W_ASSERT_FMT(it != entities_.end(), "Unknown entity ID: %lu", id);
		return it->second;
	}

	// Add a component dynamically, use this instead of create_component() when entity is already submitted
	template <typename ComponentT>
	static inline ComponentT& add_component(EntityID entity_id, void* p_component_description=nullptr)
	{
		auto& ent = get_entity(entity_id);
		auto& cmp = create_component<ComponentT>(entity_id, p_component_description);
		for(auto&& psys: systems_)
			psys->on_entity_updated(ent);
		return cmp;
	}

	static void shutdown();

private:
	friend class Application;

	// * Helpers
	// Retrieve component manager index from component type
	template <typename ComponentT>
	static inline size_t get_component_manager_index()
	{
		auto it = components_lookup_.find(ComponentT::ID);
		W_ASSERT_FMT(it != components_lookup_.end(), "No component manager for component ID: %lu", ComponentT::ID);
		return it->second;
	}
	// Retrieve component manager and cast to correct type
	template <typename ComponentT>
	static inline ComponentManager<ComponentT>* get_component_manager()
	{
		using CMgrT = ComponentManager<ComponentT>;
		auto it = components_lookup_.find(ComponentT::ID);
		W_ASSERT_FMT(it != components_lookup_.end(), "No component manager for component ID: %lu", ComponentT::ID);
		size_t mgr_index = it->second;
		return static_cast<CMgrT*>(components_[mgr_index]);
	}

private:
	using Entities   = eastl::hash_map<EntityID, Entity>;
	using Systems    = eastl::vector<BaseComponentSystem*>;
	using Components = eastl::vector<BaseComponentManager*>;
	using Lookup     = eastl::hash_map<ComponentID, size_t>;

	static Entities entities_;
	static Systems systems_;
	static Components components_;
	static Lookup components_lookup_;

	static EntityID s_next_entity_id;
};

using ECS = EntityManager;

} // namespace erwin