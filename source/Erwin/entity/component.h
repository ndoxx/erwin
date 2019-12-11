#pragma once

#include "EASTL/numeric_limits.h"
#include "entity/entity_types.h"
#include "memory/arena.h"
#include "utils/ct_counter.h"

namespace erwin
{

class Component
{
public:
	Component();

	virtual bool init(void* description) = 0;

private:
	inline EntityID get_parent_entity() const      { return parent_; }
	inline uint64_t get_pool_index() const         { return pool_index_; }
	inline void set_parent_entity(EntityID entity) { parent_ = entity; }
	inline void set_pool_index(uint64_t index)     { pool_index_ = index; }

private:
	EntityID parent_;
	uint64_t pool_index_;
};

#define POOL_DECLARATION( DEFAULT_COUNT ) \
	public: \
		static PoolArena* s_ppool_; \
		static void init_pool(void* begin, size_t max_count = DEFAULT_COUNT , const char* debug_name = nullptr ); \
		static void destroy_pool(); \
	private:

#ifdef W_DEBUG
	#define POOL_DEFINITION( COMPONENT_NAME ) \
		void COMPONENT_NAME::init_pool(void* begin, size_t max_count, const char* debug_name) \
		{ \
			W_ASSERT(s_ppool_==nullptr, "Memory pool is already initialized."); \
			s_ppool_ = new PoolArena(begin, sizeof(COMPONENT_NAME), max_count, PoolArena::DECORATION_SIZE); \
			if(debug_name) \
				s_ppool_->set_debug_name(debug_name); \
			else \
				s_ppool_->set_debug_name(#COMPONENT_NAME); \
		} \
		void COMPONENT_NAME::destroy_pool() \
		{ \
			delete s_ppool_; \
		}
#else
	#define POOL_DEFINITION( COMPONENT_NAME ) \
		void COMPONENT_NAME::init_pool(void* begin, size_t max_count, const char* debug_name) \
		{ \
			W_ASSERT(s_ppool_==nullptr, "Memory pool is already initialized."); \
			s_ppool_ = new PoolArena(begin, sizeof(COMPONENT_NAME), max_count, PoolArena::DECORATION_SIZE); \
		} \
		void COMPONENT_NAME::destroy_pool() \
		{ \
			delete s_ppool_; \
		}
#endif

} // namespace erwin