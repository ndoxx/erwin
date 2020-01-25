#pragma once

/*
	Original design by Rez Bot: https://www.youtube.com/watch?v=39e1qpsutBU
	Major modifications:
		- Component filtering is put in a policy template which is a parameter of the concrete component system.
		- virtual render() function, needed for my engine to be able to dispatch render calls
		  when appropriate (not during the update phase).

	Template class the concrete component systems inherit from. Concrete component systems
	will only care about the components specified as template parameters of the filter.

	Usage:
	class RenderSystem2D: public ComponentSystem<RequireAll<ComponentRenderable2D, ComponentTransform2D>>
	{
		using BaseType = ComponentSystem<RequireAll<ComponentRenderable2D, ComponentTransform2D>>;

	public:
		RenderSystem2D(EntityManager* manager): BaseType(manager) {}
		virtual bool init() override final;

		virtual void update(const GameClock& clock) override final
		{
			for(auto&& cmp_tuple: components_)
			{
				ComponentTransform2D* ptransform = eastl::get<ComponentTransform2D*>(cmp_tuple);
				ComponentRenderable2D* prenderable = eastl::get<ComponentRenderable2D*>(cmp_tuple);

				// do stuff
			}
		}
	};

*/

#include "entity/base_component_system.h"
#include "entity/entity.h"
#include "EASTL/vector.h"
#include "EASTL/tuple.h"

namespace erwin
{
template <typename... CompsT>
struct RequireAll
{
	using ComponentTuple     = eastl::tuple<eastl::add_pointer_t<CompsT>...>; // Tuple of component pointers
	using Components         = eastl::vector<ComponentTuple/*, PooledEastlAllocator*/>;
	using EntityIdToIndexMap = eastl::hash_map<EntityID, size_t, eastl::hash<EntityID>, eastl::equal_to<EntityID>/*, PooledEastlAllocator*/>;

	static void filter(const Entity& entity, Components& components, EntityIdToIndexMap& entity_id_to_index_map)
	{
		// Loop through all entity's components
		ComponentTuple ctuple;
		size_t match_count = 0;
		for(auto&& [id, pcmp]: entity.get_components())
		{
			// If the entity has all the components this system requires, register the components
			if(process_entity_component<0, CompsT...>(id, pcmp, ctuple))
			{
				if(++match_count == sizeof...(CompsT))
				{
					components.emplace_back(std::move(ctuple));
					entity_id_to_index_map.emplace(entity.get_id(), components.size()-1);
					break;
				}
			}
		}
	}

private:
	// Recursive component types lookup
	template<size_t INDEX, typename CompT, typename... CompArgsT>
	static bool process_entity_component(ComponentID id, Component* pcmp, ComponentTuple& target_tuple)
	{
		if(CompT::ID == id)
		{
			eastl::get<INDEX>(target_tuple) = static_cast<CompT*>(pcmp);
			return true;
		}
		else
			return process_entity_component<INDEX+1, CompArgsT...>(id, pcmp, target_tuple);
	}

	// Termination specialization of recursion
	template<size_t INDEX>
	static bool process_entity_component(ComponentID id, Component* pcmp, ComponentTuple& target_tuple)
	{
		return false;
	}
};



template <typename FilterT>
class ComponentSystem: public BaseComponentSystem
{
public:
	explicit ComponentSystem(EntityManager* manager): BaseComponentSystem(manager) {}
	virtual ~ComponentSystem() = default;

	virtual void on_entity_submitted(const Entity& entity) override final;
	virtual void on_entity_destroyed(const Entity& entity) override final;

	virtual void update(const GameClock& clock) override = 0;
	virtual void render() override { }
	virtual bool init() = 0;

	using BaseType = ComponentSystem<FilterT>;

protected:
	using ComponentTuple = typename FilterT::ComponentTuple;
	using Components     = typename FilterT::Components;

	Components components_;

private:
	using EntityIdToIndexMap = typename FilterT::EntityIdToIndexMap;

	EntityIdToIndexMap entity_id_to_index_map_;
};

template <typename FilterT>
void ComponentSystem<FilterT>::on_entity_submitted(const Entity& entity)
{
	FilterT::filter(entity, components_, entity_id_to_index_map_);
}

template <typename FilterT>
void ComponentSystem<FilterT>::on_entity_destroyed(const Entity& entity)
{
	auto it = entity_id_to_index_map_.find(entity.get_id());
	if(it != entity_id_to_index_map_.end())
	{
		// Move back element to overwrite this one (swap trick for fast deletion)
		components_[it->second] = std::move(components_.back());
		components_.pop_back();
		// Figure out which entity the moved components belongs to,
		// in order to update the entity to index map.
		// Simply grab first component in tuple, as all components in this
		// tuple belong to the same entity.
		const auto* pmoved = eastl::get<0>(components_[it->second]);
		auto moved_it = entity_id_to_index_map_.find(pmoved->get_parent_entity());
		// Should never happen
		W_ASSERT(moved_it != entity_id_to_index_map_.end(), "Cannot find entity entry for moved component.");
		moved_it->second = it->second;
	}
}


} // namespace erwin