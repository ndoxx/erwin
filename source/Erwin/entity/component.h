#pragma once

#include <string>
#include "EASTL/numeric_limits.h"
#include "entity/entity_types.h"
#include "memory/arena.h"
#include "ctti/type_id.hpp"

namespace erwin
{

class Component
{
public:
	friend class EntityManager;
	
	Component(): parent_(k_invalid_entity_id), pool_index_(k_invalid_pool_index) {}
	virtual ~Component() = default;

	virtual bool init(void* description) = 0;
	virtual const std::string& get_debug_name() const = 0;

	inline EntityID get_parent_entity() const      { return parent_; }
	inline uint64_t get_pool_index() const         { return pool_index_; }

private:
	inline void set_parent_entity(EntityID entity) { parent_ = entity; }
	inline void set_pool_index(uint64_t index)     { pool_index_ = index; }

private:
	EntityID parent_;
	uint64_t pool_index_;
};

#define DEFAULT_COMPONENT_COUNT 32

// Macro to generate the declarations of:
// - Static component ID and debug name
// - Component pool creation/destruction functions
// - Operator overloads for new and delete
// MUST be called in public
#define COMPONENT_DECLARATION( COMPONENT_NAME ) \
	static constexpr ComponentID ID = ctti::type_id< COMPONENT_NAME >().hash(); \
	static std::string NAME; \
	static PoolArena s_pool_; \
	virtual const std::string& get_debug_name() const override { return NAME; } \
    static void init_pool(memory::HeapArea& area, size_t node_size, size_t max_count, const char* debug_name); \
	static void destroy_pool(); \
	static void* operator new(size_t size); \
	static void operator delete(void* ptr)

// Macro to generate the definitions of symbols declared by previous macro
#define COMPONENT_DEFINITION( COMPONENT_NAME ) \
	std::string COMPONENT_NAME::NAME = #COMPONENT_NAME; \
	PoolArena COMPONENT_NAME::s_pool_; \
	void COMPONENT_NAME::init_pool(memory::HeapArea& area, size_t node_size, size_t max_count, const char* debug_name) \
	{ \
		W_ASSERT_FMT(!s_pool_.is_initialized(), "Memory pool for %s is already initialized.", #COMPONENT_NAME); \
		s_pool_.init(area, sizeof(COMPONENT_NAME) + PoolArena::DECORATION_SIZE, max_count, debug_name); \
		if(debug_name) \
			s_pool_.set_debug_name(debug_name); \
		else \
			s_pool_.set_debug_name(#COMPONENT_NAME); \
	} \
	void COMPONENT_NAME::destroy_pool() \
	{ \
		s_pool_.shutdown(); \
	} \
	void* COMPONENT_NAME::operator new(size_t size) \
	{ \
		(void)(size); \
		W_ASSERT_FMT(s_pool_.is_initialized(), "Memory pool for %s has not been created yet. Call init_pool().", #COMPONENT_NAME); \
		return ::W_NEW( COMPONENT_NAME , s_pool_ ); \
	} \
	void COMPONENT_NAME::operator delete(void* ptr) \
	{ \
		W_ASSERT_FMT(s_pool_.is_initialized(), "Memory pool for %s has not been created yet. Call init_pool().", #COMPONENT_NAME); \
		W_DELETE( (COMPONENT_NAME*)(ptr) , s_pool_ ); \
	}

} // namespace erwin