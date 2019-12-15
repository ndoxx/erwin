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

namespace erwin
{

class Component;
class BaseComponentSystem;
class GameClock;
class EntityManager
{
public:
	NON_COPYABLE(EntityManager);
	NON_MOVABLE(EntityManager);
	EntityManager();
	~EntityManager();

	// Create a component system, only enabled for types inheritting from BaseComponentSystem
	template <typename SystemT, typename = typename std::enable_if<std::is_base_of<BaseComponentSystem, SystemT>::value, SystemT>::type>
	SystemT* create_system()
	{
		SystemT* psys = new SystemT(this);
		systems_.push_back(psys);
		return psys;
	}

	// Create a component container and initialize component pool, only enabled for types inheritting from Component
	template <typename ComponentT, typename = typename std::enable_if<std::is_base_of<Component, ComponentT>::value, ComponentT>::type>
	void create_component_manager(memory::HeapArea& area, size_t max_components)
	{
		auto* pmgr = new ComponentManager<ComponentT>(area, max_components);
		size_t index = components_.size();
		components_.push_back(pmgr);
		components_lookup_.emplace(ComponentT::ID, index);
	}

	void update(const GameClock& clock);

	EntityID create_entity();

	template <typename ComponentT>
	ComponentT& create_component(EntityID entity_id)
	{
		// Create component
		auto* pcmp = get_component_manager<ComponentT>()->create_component(entity_id);
		// Register component in entity (TMP?)
		Entity& entity = get_entity(entity_id);
		entity.add_component(pcmp);
		return *pcmp;
	}

	void submit_entity(EntityID entity_id);
	void destroy_entity(EntityID entity_id);

	inline Entity& get_entity(EntityID id)
	{
		auto it = entities_.find(id);
		W_ASSERT_FMT(it != entities_.end(), "Unknown entity ID: %lu", id);
		return it->second;
	}

private:
	template <typename ComponentT>
	inline ComponentManager<ComponentT>* get_component_manager()
	{
		// Retrieve component manager and cast to correct type
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

	Entities entities_;
	Systems systems_;
	Components components_;

	eastl::hash_map<ComponentID, size_t> components_lookup_;

	static EntityID s_next_entity_id;
};


} // namespace erwin