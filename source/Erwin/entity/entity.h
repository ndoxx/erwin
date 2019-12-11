#pragma once

#include <cstdint>
#include "EASTL/hash_map.h"
#include "entity/component.h"

namespace erwin
{

class Entity
{
public:
	explicit Entity(EntityID id);

	inline EntityID get_id() const { return id_; }

	template <typename ComponentT>
	inline ComponentT* get_component() const
	{
		auto it = components_.find(ComponentT::ID);
		if(it != components_.end())
			return it->second;
		return nullptr;
	}

private:
	eastl::hash_map<ComponentID, Component*> components_;
	EntityID id_;
};


} // namespace erwin