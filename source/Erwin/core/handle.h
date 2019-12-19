#pragma once

namespace erwin
{

typedef uint64_t HandleID;
static constexpr uint16_t k_invalid_handle = 0xffff;
static constexpr size_t   k_max_handles = 128;

#define HANDLE_DECLARATION(HANDLE_NAME) \
	struct HANDLE_NAME \
	{ \
		static constexpr HandleID ID = ctti::type_id< HANDLE_NAME >().hash(); \
		static HandlePool* s_ppool_; \
		static void init_pool(LinearArena& arena); \
		static void destroy_pool(LinearArena& arena); \
		static HANDLE_NAME acquire(); \
		inline void release()  { s_ppool_->release(index); index = k_invalid_handle; } \
		inline bool is_valid() { return s_ppool_->is_valid(index); } \
		inline bool operator ==(const HANDLE_NAME& o) const { return index==o.index; } \
		inline bool operator !=(const HANDLE_NAME& o) const { return index!=o.index; } \
		uint32_t index = k_invalid_handle; \
	}

#define HANDLE_DEFINITION(HANDLE_NAME) \
	HandlePool* HANDLE_NAME::s_ppool_ = nullptr; \
	void HANDLE_NAME::init_pool(LinearArena& arena) \
	{ \
		W_ASSERT_FMT(s_ppool_==nullptr, "Memory pool for %s is already initialized.", #HANDLE_NAME); \
		s_ppool_ = W_NEW(HandlePoolT<k_max_handles>, arena); \
	} \
	void HANDLE_NAME::destroy_pool(LinearArena& arena) \
	{ \
		W_DELETE(s_ppool_, arena); \
	} \
	HANDLE_NAME HANDLE_NAME::acquire() \
	{ \
		return HANDLE_NAME{ s_ppool_->acquire() }; \
	}

} // namespace erwin