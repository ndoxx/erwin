#pragma once

#include <cstdint>
#include "ctti/type_id.hpp"
#include "memory/handle_pool.h"
#include "memory/arena.h"

namespace erwin
{

typedef uint64_t HandleID;
static constexpr uint16_t k_invalid_handle = 0xffff;
static constexpr uint64_t k_invalid_robust_handle = 0xffffffffffffffff;

#define HANDLE_DECLARATION(HANDLE_NAME) \
	struct HANDLE_NAME \
	{ \
		static constexpr HandleID ID = ctti::type_id< HANDLE_NAME >().hash(); \
		static HandlePool* s_ppool_; \
		static void init_pool(LinearArena& arena); \
		static void destroy_pool(LinearArena& arena); \
		static HANDLE_NAME acquire(); \
		inline void release()  { s_ppool_->release(index); } \
		inline bool is_valid() const { return s_ppool_->is_valid(index); } \
		inline bool operator ==(const HANDLE_NAME& o) const { return index==o.index; } \
		inline bool operator !=(const HANDLE_NAME& o) const { return index!=o.index; } \
		uint16_t index = k_invalid_handle; \
	}

#define HANDLE_DEFINITION(HANDLE_NAME, MAX_HANDLES) \
	HandlePool* HANDLE_NAME::s_ppool_ = nullptr; \
	void HANDLE_NAME::init_pool(LinearArena& arena) \
	{ \
		W_ASSERT_FMT(s_ppool_==nullptr, "Memory pool for %s is already initialized.", #HANDLE_NAME); \
		s_ppool_ = W_NEW(HandlePoolT< MAX_HANDLES >, arena); \
	} \
	void HANDLE_NAME::destroy_pool(LinearArena& arena) \
	{ \
		W_DELETE(s_ppool_, arena); \
		s_ppool_ = nullptr; \
	} \
	HANDLE_NAME HANDLE_NAME::acquire() \
	{ \
		return HANDLE_NAME{ s_ppool_->acquire() }; \
	}



#define ROBUST_HANDLE_DECLARATION(HANDLE_NAME) \
	struct HANDLE_NAME \
	{ \
		static constexpr HandleID ID = ctti::type_id< HANDLE_NAME >().hash(); \
		static RobustHandlePool* s_ppool_; \
		static void init_pool(LinearArena& arena); \
		static void destroy_pool(LinearArena& arena); \
		static HANDLE_NAME acquire(); \
		inline void release()  { s_ppool_->release({index,counter}); } \
		inline bool is_valid() const { return s_ppool_->is_valid({index,counter}); } \
		inline bool operator ==(const HANDLE_NAME& o) const { return index==o.index && counter==o.counter; } \
		inline bool operator !=(const HANDLE_NAME& o) const { return index!=o.index || counter!=o.counter; } \
		uint64_t index = k_invalid_robust_handle; \
		uint64_t counter = k_invalid_robust_handle; \
	}

#define ROBUST_HANDLE_DEFINITION(HANDLE_NAME, MAX_HANDLES) \
	RobustHandlePool* HANDLE_NAME::s_ppool_ = nullptr; \
	void HANDLE_NAME::init_pool(LinearArena& arena) \
	{ \
		W_ASSERT_FMT(s_ppool_==nullptr, "Memory pool for %s is already initialized.", #HANDLE_NAME); \
		s_ppool_ = W_NEW(RobustHandlePoolT< MAX_HANDLES >, arena); \
	} \
	void HANDLE_NAME::destroy_pool(LinearArena& arena) \
	{ \
		W_DELETE(s_ppool_, arena); \
		s_ppool_ = nullptr; \
	} \
	HANDLE_NAME HANDLE_NAME::acquire() \
	{ \
		auto [index, counter] = s_ppool_->acquire(); \
		return HANDLE_NAME{ index, counter }; \
	}

} // namespace erwin