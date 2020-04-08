#pragma once

#include <type_traits>
#include <map>
#include "entt/entt.hpp"
#include "debug/logger.h"

#define W_METAFUNC_GET_COMPONENT "get_component"_hs
#define W_METAFUNC_HAS_COMPONENT "has_component"_hs
#define W_METAFUNC_CREATE_COMPONENT "create_component"_hs
#define W_METAFUNC_REMOVE_COMPONENT "remove_component"_hs
#define W_METAFUNC_TRY_REMOVE_COMPONENT "try_remove_component"_hs
#define W_METAFUNC_INSPECTOR_GUI "inspector_GUI"_hs

namespace erwin
{
namespace traits
{
	template <typename FuncType>
	using IsInvocable = typename std::enable_if_t<std::is_invocable_v<FuncType, uint64_t, void*>>;
} // namespace traits

// Associate a component type ID to a reflection hash string
extern void add_reflection(uint64_t type_id, uint64_t reflected_type);

// Obtain reflection hash string from type ID
extern uint64_t reflect(uint64_t type_id);

// Obtain map of reflection hash string to component name
extern const std::map<uint64_t, std::string>& get_component_names();

// Helpers to invoke meta-functions
template <typename... Args>
static inline auto invoke(uint64_t func_name, uint64_t reflected_type, Args&&... args)
{
	return entt::resolve(reflected_type).func(func_name).invoke({}, std::forward<Args>(args)...);
}
// ASSUME: registry is always the first meta-function argument
template <typename... Args>
static inline auto invoke(uint64_t func_name, uint64_t reflected_type, entt::registry& reg, Args&&... args)
{
	return entt::resolve(reflected_type).func(func_name).invoke({}, std::ref(reg), std::forward<Args>(args)...);
}

// For each component in an entity, invoke a function or any callable type taking
// the component type ID and an opaque void* pointer to the component's instance as parameters
// Note that func must be of an invocable type
template <typename FuncType, typename = traits::IsInvocable<FuncType>>
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

// Any component that needs to be editable in an inspector should specialize this function.
template <typename ComponentType>
extern void inspector_GUI(ComponentType*) {}

using EntityID = entt::entity;
constexpr EntityID k_invalid_entity_id = entt::null;


namespace metafunc
{
	// Get a component from its associated meta-object
	template <typename ComponentType>
	static inline ComponentType& get_component(entt::registry& reg, entt::entity e)
	{
	    return reg.get<ComponentType>(e);
	}

	// Check if an entity has a specific component
	template <typename ComponentType>
	static inline bool has_component(entt::registry& reg, entt::entity e)
	{
	    return reg.has<ComponentType>(e);
	}

	// Create a component
	template <typename ComponentType>
	static inline void create_component(entt::registry& reg, entt::entity e)
	{
	    reg.assign<ComponentType>(e);
	}

	// Remove a component given its associated meta-object
	template <typename ComponentType>
	static inline void remove_component(entt::registry& reg, entt::entity e)
	{
	    reg.remove<ComponentType>(e);
	}

	// Remove a component given its associated meta-object only if it exists
	template <typename ComponentType>
	static inline void try_remove_component(entt::registry& reg, entt::entity e)
	{
	    reg.remove_if_exists<ComponentType>(e);
	}

	// Helper function to cast a component opaque pointer to the correct type
	// before calling the actual implementation.
	// This way, the invocation code can pass a void*, whereas the GUI implementation takes
	// an actual component pointer.
	template <typename ComponentType>
	inline void inspector_GUI_typecast(void* data)
	{
		inspector_GUI<ComponentType>(static_cast<ComponentType*>(data));
	}
} // namespace metafunc

} // namespace erwin

// Create a reflection meta-object for an input component type
#define REFLECT_COMPONENT(_type) \
	entt::meta< _type >() \
		.type( #_type ##_hs) \
		.prop("name"_hs, #_type ) \
		.func<&erwin::metafunc::get_component< _type >, entt::as_alias_t>(W_METAFUNC_GET_COMPONENT) \
		.func<&erwin::metafunc::has_component< _type >>(W_METAFUNC_HAS_COMPONENT) \
		.func<&erwin::metafunc::create_component< _type >, entt::as_void_t>(W_METAFUNC_CREATE_COMPONENT) \
		.func<&erwin::metafunc::remove_component< _type >, entt::as_void_t>(W_METAFUNC_REMOVE_COMPONENT) \
		.func<&erwin::metafunc::try_remove_component< _type >, entt::as_void_t>(W_METAFUNC_TRY_REMOVE_COMPONENT) \
		.func<&erwin::metafunc::inspector_GUI_typecast< _type >, entt::as_void_t>(W_METAFUNC_INSPECTOR_GUI); \
	erwin::add_reflection(entt::type_info< _type >::id(), #_type ##_hs)
