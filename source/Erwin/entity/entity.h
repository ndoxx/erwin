#pragma once

#include <cstdint>
#include "EASTL/hash_map.h"
#include "entity/component.h"
#include "core/core.h"

namespace erwin
{

class Entity
{
public:
	using ComponentMap = eastl::hash_map<ComponentID, Component*, eastl::hash<ComponentID>, eastl::equal_to<ComponentID>/*, PooledEastlAllocator*/>;

	NON_COPYABLE(Entity);
	explicit Entity(EntityID id);
	Entity(Entity&&) = default;
	Entity& operator=(Entity&&) = default;
	~Entity();

	inline EntityID get_id() const { return id_; }

	template <typename ComponentT>
	inline ComponentT* get_component() const
	{
		auto it = components_.find(ComponentT::ID);
		if(it != components_.end())
			return it->second;
		return nullptr;
	}

	inline const ComponentMap& get_components() const { return components_; }

	template <typename ComponentT>
	inline void add_component(ComponentT* pcmp)
	{
		W_ASSERT(components_.find(ComponentT::ID) == components_.end(), "Entity already has a component of this type.");
		pcmp->set_parent_entity(id_);
		pcmp->set_pool_index(0); // TMP
		components_.emplace(ComponentT::ID, pcmp);
	}

private:
	ComponentMap components_;
	EntityID id_;
};


} // namespace erwin