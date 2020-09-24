#pragma once

#include "ctti/type_id.hpp"
#include "memory/arena.h"
#include "memory/handle_pool.h"
#include "render/renderer_config.h"
#include <cstdint>
#include <ostream>

namespace erwin
{

// Maximum amount of handles for every managed object
// Default is 256, but the HANDLE_DECLARATION() macro overrides this setting
template <typename HandleT>[[maybe_unused]] static constexpr uint32_t k_max_handles = 256;

typedef uint64_t HandleID;
[[maybe_unused]] static constexpr uint32_t k_null_handle = 0xffffffff;

#define HANDLE_DECLARATION(HANDLE_NAME, MAX_HANDLES)                                                                   \
    struct HANDLE_NAME                                                                                                 \
    {                                                                                                                  \
        using PoolT = HandlePoolT<MAX_HANDLES>;                                                                        \
        static constexpr HandleID ID = ctti::type_id<HANDLE_NAME>().hash();                                            \
        static PoolT* s_ppool_;                                                                                        \
        static void init_pool(LinearArena& arena);                                                                     \
        static void destroy_pool(LinearArena& arena);                                                                  \
        static HANDLE_NAME acquire();                                                                                  \
        inline void release()                                                                                          \
        {                                                                                                              \
            s_ppool_->release(data);                                                                                   \
            data = k_null_handle;                                                                                      \
        }                                                                                                              \
        inline uint16_t index() const { return data & PoolT::k_handle_mask; }                                          \
        inline uint16_t guard() const { return (data & PoolT::k_guard_mask) >> PoolT::k_guard_shift; }                 \
        inline bool is_valid() const { return s_ppool_->is_valid(data); }                                              \
        inline bool is_null() const { return data == k_null_handle; }                                                  \
        inline bool operator==(const HANDLE_NAME& o) const { return data == o.data; }                                  \
        inline bool operator!=(const HANDLE_NAME& o) const { return data != o.data; }                                  \
        friend std::ostream& operator<<(std::ostream& stream, const HANDLE_NAME&);                                     \
        uint32_t data = k_null_handle;                                                                                 \
    };                                                                                                                 \
    template <> static constexpr uint32_t k_max_handles<HANDLE_NAME> = MAX_HANDLES

#define HANDLE_DEFINITION(HANDLE_NAME)                                                                                 \
    HANDLE_NAME::PoolT* HANDLE_NAME::s_ppool_ = nullptr;                                                               \
    void HANDLE_NAME::init_pool(LinearArena& arena)                                                                    \
    {                                                                                                                  \
        W_ASSERT_FMT(s_ppool_ == nullptr, "Memory pool for %s is already initialized.", #HANDLE_NAME);                 \
        s_ppool_ = W_NEW(HandlePoolT<k_max_handles<HANDLE_NAME>>, arena);                                              \
    }                                                                                                                  \
    void HANDLE_NAME::destroy_pool(LinearArena& arena)                                                                 \
    {                                                                                                                  \
        W_DELETE(s_ppool_, arena);                                                                                     \
        s_ppool_ = nullptr;                                                                                            \
    }                                                                                                                  \
    std::ostream& operator<<(std::ostream& stream, const HANDLE_NAME& h)                                               \
    {                                                                                                                  \
        stream << '[' << h.index() << '/' << h.guard() << ']';                                                         \
        return stream;                                                                                                 \
    }                                                                                                                  \
    HANDLE_NAME HANDLE_NAME::acquire() { return HANDLE_NAME{s_ppool_->acquire()}; }

} // namespace erwin