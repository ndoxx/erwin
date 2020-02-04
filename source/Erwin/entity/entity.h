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

	inline EntityID get_id() const             { return id_; }
	inline const std::string& get_name() const { return name_; }
	inline const char* get_icon() const        { return icon_.c_str(); }

	inline void set_name(const std::string& name) { name_ = name; }
	inline void set_icon(const char* icon)        { icon_ = icon;}

	template <typename ComponentT>
	inline void add_component(ComponentT* pcmp) { components_.emplace(ComponentT::ID, pcmp); }

	template <typename ComponentT>
	inline bool remove_component()
	{
		auto it = components_.find(ComponentT::ID);
		if(it!= components_.end())
		{
			components_.erase(it);
			return true;
		}
		return false;
	}

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
	std::string icon_;
	std::string name_;
};


} // namespace erwin