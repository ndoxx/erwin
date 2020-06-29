#pragma once

#include <type_traits>
#include <map>
#include <istream>
#include <ostream>
#include "entt/entt.hpp"
#include "debug/logger.h"

#include "filesystem/xml_file.h"

#define W_METAFUNC_GET_COMPONENT "get_component"_hs
#define W_METAFUNC_HAS_COMPONENT "has_component"_hs
#define W_METAFUNC_CREATE_COMPONENT "create_component"_hs
#define W_METAFUNC_REMOVE_COMPONENT "remove_component"_hs
#define W_METAFUNC_TRY_REMOVE_COMPONENT "try_remove_component"_hs
#define W_METAFUNC_INSPECTOR_GUI "inspector_GUI"_hs
#define W_METAFUNC_SERIALIZE_XML "serialize_xml"_hs
#define W_METAFUNC_SERIALIZE_BIN "serialize_bin"_hs
#define W_METAFUNC_DESERIALIZE_XML "deserialize_xml"_hs
#define W_METAFUNC_DESERIALIZE_BIN "deserialize_bin"_hs

namespace erwin
{
namespace traits
{
	template <typename FuncType>
	using IsInvocable = typename std::enable_if_t<std::is_invocable_v<FuncType, uint64_t, void*>>;
} // namespace traits

// Associate a component type ID to a reflection hash string
extern void add_reflection(uint64_t type_id, uint32_t reflected_type);

// Hide a component type from inspector
extern void hide_from_inspector(uint32_t reflected_type);

// Check if a component type is hidden from inspection
extern bool is_hidden_from_inspector(uint32_t reflected_type);

// Obtain reflection hash string from type ID
extern uint32_t reflect(uint64_t type_id);

// Obtain map of reflection hash string to component name
extern const std::map<uint32_t, std::string>& get_component_names();

// Helpers to invoke meta-functions
template <typename... Args>
static inline auto invoke(uint32_t func_name, uint32_t reflected_type, Args&&... args)
{
	return entt::resolve_id(reflected_type).func(func_name).invoke({}, std::forward<Args>(args)...);
}
// ASSUME: registry is always the first meta-function argument
template <typename... Args>
static inline auto invoke(uint32_t func_name, uint32_t reflected_type, entt::registry& reg, Args&&... args)
{
	return entt::resolve_id(reflected_type).func(func_name).invoke({}, std::ref(reg), std::forward<Args>(args)...);
}

// For each component in an entity, invoke a function or any callable type taking
// the component type ID and an opaque void* pointer to the component's instance as parameters
// Note that func must be of an invocable type
template <typename FuncType, typename = traits::IsInvocable<FuncType>>
static inline void visit_entity(entt::registry& reg, entt::entity e, FuncType&& func)
{
	reg.visit(e, [&reg,&func,e](uint64_t type_id)
	{
		uint32_t reflected_type = reflect(type_id);
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
[[maybe_unused]] static inline void visit_entity(entt::registry& reg, entt::entity e, uint32_t meta_func)
{
	reg.visit(e, [&reg,meta_func,e](uint64_t type_id)
	{
		uint32_t reflected_type = reflect(type_id);
		if(reflected_type==0)
			return;
		auto any = invoke(W_METAFUNC_GET_COMPONENT, reflected_type, reg, e);
		if(any)
			invoke(meta_func, reflected_type, any.data());
	});
}

using EntityID = entt::entity;
[[maybe_unused]] static constexpr EntityID k_invalid_entity_id = entt::null;

// Any component that needs to be editable in an inspector should specialize this function.
template <typename ComponentType>
extern void inspector_GUI(ComponentType&, EntityID, entt::registry&) {}

// Serialization
template <typename ComponentType>
extern void serialize_xml(const ComponentType&, xml::XMLFile&, rapidxml::xml_node<>*) {}

template <typename ComponentType>
extern void serialize_bin(const ComponentType&, std::ostream&) {}

// Deserialization
template <typename ComponentType>
extern void deserialize_xml(rapidxml::xml_node<>*, EntityID, entt::registry&) {}

template <typename ComponentType>
extern void deserialize_bin(std::istream&, EntityID, entt::registry&) {}

namespace metafunc
{
	// Check if an entity has a specific component
	template <typename ComponentType>
	static inline bool has_component(const entt::registry& reg, entt::entity e)
	{
	    return reg.has<ComponentType>(e);
	}
	
	// Get a component from its associated meta-object
	template <typename ComponentType>
	static inline ComponentType& get_component(entt::registry& reg, entt::entity e)
	{
	    return reg.get<ComponentType>(e);
	}

	// Create a component
	template <typename ComponentType>
	static inline void create_component(entt::registry& reg, entt::entity e)
	{
	    reg.emplace<ComponentType>(e);
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
	// a concrete component reference.
	template <typename ComponentType>
	inline void inspector_GUI_typecast(void* cmp, EntityID e, entt::registry* r)
	{
		inspector_GUI<ComponentType>(*static_cast<ComponentType*>(cmp), e, *r);
	}

	template <typename ComponentType>
	inline void serialize_xml_typecast(void* cmp, xml::XMLFile* file, void* cmp_node)
	{
		serialize_xml<ComponentType>(*static_cast<const ComponentType*>(cmp), *file, static_cast<rapidxml::xml_node<>*>(cmp_node));
	}

	template <typename ComponentType>
	inline void serialize_bin_typecast(void* cmp, std::ostream& stream)
	{
		serialize_bin<ComponentType>(*static_cast<const ComponentType*>(cmp), stream);
	}

	template <typename ComponentType>
	inline void deserialize_xml_typecast(void* node, EntityID e, entt::registry* r)
	{
		deserialize_xml<ComponentType>(static_cast<rapidxml::xml_node<>*>(node), e, *r);
	}

	template <typename ComponentType>
	inline void deserialize_bin_typecast(std::istream& stream, EntityID e, entt::registry* r)
	{
		deserialize_bin<ComponentType>(stream, e, *r);
	}
} // namespace metafunc
} // namespace erwin

// Create a reflection meta-object for an input component type
#define REFLECT_COMPONENT( CTYPE ) \
	entt::meta< CTYPE >() \
		.type( #CTYPE ##_hs ) \
		.prop("name"_hs, #CTYPE ) \
		.func<&erwin::metafunc::get_component< CTYPE >, entt::as_ref_t>(W_METAFUNC_GET_COMPONENT) \
		.func<&erwin::metafunc::has_component< CTYPE >>(W_METAFUNC_HAS_COMPONENT) \
		.func<&erwin::metafunc::create_component< CTYPE >, entt::as_void_t>(W_METAFUNC_CREATE_COMPONENT) \
		.func<&erwin::metafunc::remove_component< CTYPE >, entt::as_void_t>(W_METAFUNC_REMOVE_COMPONENT) \
		.func<&erwin::metafunc::try_remove_component< CTYPE >, entt::as_void_t>(W_METAFUNC_TRY_REMOVE_COMPONENT) \
		.func<&erwin::metafunc::inspector_GUI_typecast< CTYPE >, entt::as_void_t>(W_METAFUNC_INSPECTOR_GUI) \
		.func<&erwin::metafunc::serialize_xml_typecast< CTYPE >, entt::as_void_t>(W_METAFUNC_SERIALIZE_XML) \
		.func<&erwin::metafunc::serialize_bin_typecast< CTYPE >, entt::as_void_t>(W_METAFUNC_SERIALIZE_BIN) \
		.func<&erwin::metafunc::deserialize_xml_typecast< CTYPE >, entt::as_void_t>(W_METAFUNC_DESERIALIZE_XML) \
		.func<&erwin::metafunc::deserialize_bin_typecast< CTYPE >, entt::as_void_t>(W_METAFUNC_DESERIALIZE_BIN); \
	erwin::add_reflection(entt::type_info< CTYPE >::id(), #CTYPE ##_hs)

#define HIDE_FROM_INSPECTOR( CTYPE ) erwin::hide_from_inspector( #CTYPE ##_hs )
