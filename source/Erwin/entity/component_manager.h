#pragma once

#include "entity/entity_types.h"
#include "memory/arena.h"
#include "core/core.h"
#include "EASTL/hash_map.h"
#include "EASTL/vector.h"

namespace erwin
{

class BaseComponentManager
{
public:
	virtual ~BaseComponentManager() = default;
	virtual void remove(EntityID entity_id) = 0;
	virtual bool contains(EntityID entity_id) = 0;
	virtual size_t get_count() = 0;
};

/*
	ComponentManager is a container that owns the components of its template type.
	This class also associates each component to its the parent entity.
*/
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

	ComponentT* create(EntityID entity_id)
	{
		ComponentT* pcmp = new ComponentT();
		W_ASSERT_FMT(lookup_.find(entity_id) == lookup_.end(), "Entity %lu already has a component of this type: %s", entity_id, ComponentT::NAME.c_str());

		size_t index = components_.size();
		components_.push_back(pcmp);
		lookup_.emplace(entity_id, index);

		return pcmp;
	}

	virtual void remove(EntityID entity_id) override
	{
		auto it = lookup_.find(entity_id);
		W_ASSERT_FMT(it != lookup_.end(), "Cannot find entity %lu in %s manager", entity_id, ComponentT::NAME.c_str());

		// Use swap trick for fast removal
		size_t index = it->second;
		delete components_[index];
		components_[index] = std::move(components_.back());
		components_.pop_back();
		size_t back_entity_id = components_[index]->get_parent_entity();
		// Update lookup table at index
		lookup_[back_entity_id] = index;
	}

	virtual bool contains(EntityID entity_id) override
	{
		return (lookup_.find(entity_id) != lookup_.end());
	}

	virtual size_t get_count() override
	{
		return components_.size();
	}

private:
	eastl::vector<ComponentT*> components_;
	eastl::hash_map<EntityID, size_t> lookup_;
};


} // namespace erwin