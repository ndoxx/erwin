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
	explicit Entity(EntityID id): id_(id) { }
	Entity(Entity&&) = default;
	Entity& operator=(Entity&&) = default;
	~Entity() = default;

	inline EntityID get_id() const { return id_; }

	template <typename ComponentT>
	inline void add_component(ComponentT* pcmp) { components_.emplace(ComponentT::ID, pcmp); }

	template <typename ComponentT>
	inline ComponentT* get_component() const
	{
		auto it = components_.find(ComponentT::ID);
		if(it != components_.end())
			return static_cast<ComponentT*>(it->second);
		return nullptr;
	}

	inline const ComponentMap& get_components() const { return components_; }

private:
	ComponentMap components_;
	EntityID id_;
};


} // namespace erwin