#pragma once
#include <string>
#include <type_traits>
#include "EASTL/vector.h"
#include "EASTL/hash_map.h"

#include "core/handle.h"
#include "ctti/type_id.hpp"

namespace erwin
{
ROBUST_HANDLE_DECLARATION(EntityHandle);
}

namespace wip
{

using ComponentID = uint64_t;
using EntityIdx = uint64_t;

constexpr size_t k_max_entities = 4096;


class Component
{
public:
	Component() = default;
	virtual ~Component() = default;

	virtual bool init(void* description) { return true; }

#ifdef W_DEBUG
	virtual const std::string& get_debug_name() const = 0;
	virtual void inspector_GUI() { }
#endif

	inline erwin::EntityHandle get_parent_entity() const { return parent_; }

private:
	inline void set_parent_entity(erwin::EntityHandle entity) { parent_ = entity; }

private:
	erwin::EntityHandle parent_;
};

#ifdef W_DEBUG
	#define COMPONENT_DECLARATION( COMPONENT_NAME ) \
		static constexpr ComponentID ID = ctti::type_id< COMPONENT_NAME >().hash(); \
		static std::string NAME; \
		virtual const std::string& get_debug_name() const override { return NAME; }

	#define COMPONENT_DEFINITION( COMPONENT_NAME ) \
		std::string COMPONENT_NAME::NAME = #COMPONENT_NAME
#else
	#define COMPONENT_DECLARATION( COMPONENT_NAME ) \
		static constexpr ComponentID ID = ctti::type_id< COMPONENT_NAME >().hash();

	#define COMPONENT_DEFINITION( COMPONENT_NAME )
#endif


class ComponentStorage
{
public:

	ComponentStorage(erwin::memory::HeapArea& area, size_t max_components, size_t component_size, const std::string& debug_name);
	~ComponentStorage();

	template <typename ComponentT>
	inline ComponentT* create(erwin::EntityHandle entity)
	{
		W_ASSERT_FMT(lookup_.find(entity.index) == lookup_.end(), "Entity %lu already has a component of this type: %s", entity.index, ComponentT::NAME.c_str());
		
		ComponentT* pcmp = W_NEW(ComponentT, pool_);

		size_t index = components_.size();
		components_.push_back(static_cast<Component*>(pcmp));
		lookup_.emplace(entity.index, index);

		return pcmp;
	}

	void remove(erwin::EntityHandle entity);

	inline bool contains(erwin::EntityHandle entity)
	{
		return (lookup_.find(entity.index) != lookup_.end());
	}

	inline size_t get_count()
	{
		return components_.size();
	}

private:
	erwin::PoolArena pool_;
	eastl::vector<Component*> components_;
	eastl::hash_map<EntityIdx, size_t> lookup_;
};


class ECS
{
public:
	template <typename ComponentT>
	using EnableIfIsComponent = typename std::enable_if<std::is_base_of<Component, ComponentT>::value, ComponentT>::type;


	static erwin::EntityHandle create_entity();
	static void destroy_entity(erwin::EntityHandle entity);

	template <typename ComponentT, typename = EnableIfIsComponent<ComponentT>>
	static inline void create_component_storage(erwin::memory::HeapArea& area, size_t max_components)
	{
		create_component_storage_impl(area, max_components, sizeof(ComponentT), ComponentT::ID, ComponentT::NAME);
	}

	template <typename ComponentT, typename = EnableIfIsComponent<ComponentT>>
	static inline ComponentT* create_component(erwin::EntityHandle entity, void* p_component_description=nullptr)
	{
		// Create and initialize component
		auto* pcmp = get_storage(ComponentT::ID).template create<ComponentT>(entity);
		pcmp->set_parent_entity(entity);
		if(p_component_description)
			pcmp->init(p_component_description);

		add_entity_component(entity, ComponentT::ID, pcmp);

		return pcmp;
	}

	template <typename ComponentT, typename = EnableIfIsComponent<ComponentT>>
	static inline ComponentT* get_component(erwin::EntityHandle entity)
	{
		W_ASSERT_FMT(entity.is_valid(), "Invalid EntityHandle: (%lu,%lu)", entity.index, entity.counter);
		return static_cast<ComponentT*>(get_component(entity, ComponentT::ID));
	}

// private:
	friend class Application;

	static void init(erwin::memory::HeapArea& area);
	static void shutdown();

private:
	static void create_component_storage_impl(erwin::memory::HeapArea& area, size_t max_components, size_t component_size, ComponentID cid, const std::string& debug_name);
	static ComponentStorage& get_storage(ComponentID cid);
	static void add_entity_component(erwin::EntityHandle entity, ComponentID cid, Component* pcmp);
	static Component* get_component(erwin::EntityHandle entity, ComponentID cid);
};


} // namespace wip