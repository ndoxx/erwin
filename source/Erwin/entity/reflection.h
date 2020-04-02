#pragma once

#include <type_traits>
#include "entt/entt.hpp"
#include "debug/logger.h"

#define W_METAFUNC_GET_COMPONENT "get_component"_hs
#define W_METAFUNC_REMOVE_COMPONENT "remove_component"_hs
#define W_METAFUNC_TRY_REMOVE_COMPONENT "try_remove_component"_hs
#define W_METAFUNC_INSPECTOR_GUI "inspector_GUI"_hs

namespace erwin
{
namespace traits
{
	template <typename FuncType>
	using EnableIfIsInvocable = typename std::enable_if<std::is_invocable<FuncType>::type>;
} // namespace traits

// Associate a component type ID to a reflection hash string
extern void add_reflection(uint64_t type_id, uint64_t reflected_type);

// Obtain reflection hash string from type ID
extern uint64_t reflect(uint64_t type_id);

// Get a component from its associated meta-object
template <typename ComponentType>
static inline ComponentType& get_component(entt::registry& reg, entt::entity e)
{
    return reg.get<ComponentType>(e);
}

// Remove a component given its associated meta-object
template <typename ComponentType>
static void remove_component(entt::registry& reg, entt::entity e)
{
    reg.remove<ComponentType>(e);
}

// Remove a component given its associated meta-object only if it exists
template <typename ComponentType>
static void try_remove_component(entt::registry& reg, entt::entity e)
{
    reg.remove_if_exists<ComponentType>(e);
}

// Helpers to invoke meta-functions
[[maybe_unused]] static inline auto invoke(uint64_t func_name, uint64_t reflected_type, entt::registry& reg, entt::entity e)
{
	return entt::resolve(reflected_type).func(func_name).invoke({}, std::ref(reg), e);
}
[[maybe_unused]] static inline auto invoke(uint64_t func_name, uint64_t reflected_type, void* data)
{
	return entt::resolve(reflected_type).func(func_name).invoke({}, data);
}


// Any component that needs to be editable in an inspector should specialize this function.
// The opaque pointer is expected to be a component instance pointer, specialization is 
// responsible for the casting operation.
template <typename ComponentType>
extern void inspector_GUI(void*) {}

// For each component in an entity, invoke a function or any callable type taking
// the component type ID and an opaque void* pointer to the component's instance as parameters
template <typename FuncType/*, typename = traits::EnableIfIsInvocable<FuncType>*/>
static inline void visit_entity(entt::registry& reg, entt::entity e, FuncType&& func)
{
	reg.visit(e, [&reg,&func,e](uint64_t type_id)
	{
		uint64_t reflected_type = reflect(type_id);
		if(reflected_type==0)
			return;
		auto any = invoke(W_METAFUNC_GET_COMPONENT, reflected_type, reg, e);
		if(any)
			func(reflected_type, any.data());
	});
}

// For each component in an entity, invoke a meta-function by name.
// The meta-function should exist in the meta-object associated to each component
// this function call is susceptible to come upon.
static inline void visit_entity(entt::registry& reg, entt::entity e, uint64_t meta_func)
{
	reg.visit(e, [&reg,meta_func,e](uint64_t type_id)
	{
		uint64_t reflected_type = reflect(type_id);
		if(reflected_type==0)
			return;
		auto any = invoke(W_METAFUNC_GET_COMPONENT, reflected_type, reg, e);
		if(any)
			invoke(meta_func, reflected_type, any.data());
	});
}

using EntityID = entt::entity;
constexpr EntityID k_invalid_entity_id = entt::null;

} // namespace erwin

// Create a reflection meta-object for an input component type
#define REFLECT_COMPONENT(_type) \
	entt::meta< _type >() \
		.type( #_type ##_hs) \
		.prop("name"_hs, #_type ) \
		.func<&erwin::get_component< _type >, entt::as_alias_t>(W_METAFUNC_GET_COMPONENT) \
		.func<&erwin::remove_component< _type >, entt::as_alias_t>(W_METAFUNC_REMOVE_COMPONENT) \
		.func<&erwin::try_remove_component< _type >, entt::as_alias_t>(W_METAFUNC_TRY_REMOVE_COMPONENT) \
		.func<&erwin::inspector_GUI< _type >, entt::as_void_t>(W_METAFUNC_INSPECTOR_GUI); \
	erwin::add_reflection(entt::type_info< _type >::id(), #_type ##_hs)
