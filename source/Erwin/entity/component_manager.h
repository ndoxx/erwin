#pragma once

#include "entity/entity_types.h"
#include "memory/arena.h"
#include "EASTL/hash_map.h"
#include "EASTL/vector.h"

namespace erwin
{

class BaseComponentManager
{
public:
	virtual ~BaseComponentManager() = default;
	virtual void remove(EntityID entity_id) = 0;
};

class EntityManager;
template <typename ComponentT>
class ComponentManager: public BaseComponentManager
{
public:
	ComponentManager(memory::HeapArea& area, size_t max_components)
	{
		// Initialize component pool
		ComponentT::init_pool(area.require_pool_block<PoolArena>(sizeof(ComponentT), max_components), max_components);

	}

	virtual ~ComponentManager()
	{
		for(auto&& pcmp: components_)
			delete pcmp;

		// Destroy component pool
		ComponentT::destroy_pool();
	}

	ComponentT* create_component(EntityID entity_id)
	{
		ComponentT* pcmp = new ComponentT();

		// TODO: Check stuff

		size_t index = components_.size();
		components_.push_back(pcmp);
		entities_.push_back(entity_id);
		lookup_.emplace(entity_id, index);

		return pcmp;
	}

	virtual void remove(EntityID entity_id) override
	{
		auto it = lookup_.find(entity_id);
		if(it == lookup_.end())
			return;
		// Use swap trick for fast removal
		size_t index = it->second;
		delete components_[index];
		components_[index] = std::move(components_.back());
		components_.pop_back();
		entities_[index] = std::move(entities_.back());
		entities_.pop_back();
		size_t back_entity_id = entities_[index];
		// Update lookup table at index
		lookup_[back_entity_id] = index;
	}

private:
	eastl::vector<ComponentT*> components_;
	eastl::vector<EntityID> entities_;
	eastl::hash_map<EntityID, size_t> lookup_;
};


} // namespace erwin