#pragma once
#include "ctti/type_id.hpp"
#include "core/time_base.h"
#include "memory/arena.h"

namespace erwin
{

typedef uint64_t EventID;

#define DEFAULT_MAX_EVENT 128
#define EVENT_DECLARATION( EVENT_NAME ) \
    static std::string NAME; \
    static constexpr EventID ID = ctti::type_id< EVENT_NAME >().hash(); \
    static PoolArena* s_ppool_; \
    virtual const std::string& get_name() const override { return NAME; } \
    static void init_pool(void* begin, size_t max_count = DEFAULT_MAX_EVENT , const char* debug_name = nullptr ); \
    static void destroy_pool(); \
    static void* operator new(size_t size); \
    static void operator delete(void* ptr)

#define EVENT_DEFINITION( EVENT_NAME ) \
    std::string EVENT_NAME::NAME = #EVENT_NAME; \
    PoolArena* EVENT_NAME::s_ppool_ = nullptr; \
    void EVENT_NAME::init_pool(void* begin, size_t max_count, const char* debug_name) \
    { \
        W_ASSERT_FMT(s_ppool_==nullptr, "Memory pool for %s is already initialized.", #EVENT_NAME); \
        s_ppool_ = new PoolArena(begin, sizeof(EVENT_NAME), max_count, PoolArena::DECORATION_SIZE); \
        if(debug_name) \
            s_ppool_->set_debug_name(debug_name); \
        else \
            s_ppool_->set_debug_name(#EVENT_NAME); \
    } \
    void EVENT_NAME::destroy_pool() \
    { \
        delete s_ppool_; \
        s_ppool_ = nullptr; \
    } \
    void* EVENT_NAME::operator new(size_t size) \
    { \
        (void)(size); \
        W_ASSERT_FMT(s_ppool_, "Memory pool for %s has not been created yet. Call init_pool().", #EVENT_NAME); \
        return ::W_NEW( EVENT_NAME , (*s_ppool_) ); \
    } \
    void EVENT_NAME::operator delete(void* ptr) \
    { \
        W_ASSERT_FMT(s_ppool_, "Memory pool for %s has not been created yet. Call init_pool().", #EVENT_NAME); \
        W_DELETE( (EVENT_NAME*)(ptr) , (*s_ppool_) ); \
    }

// Base class for an event
struct WEvent
{
    WEvent(): timestamp(TimeBase::timestamp()) {}
    virtual ~WEvent() = default;

    virtual const std::string& get_name() const = 0;
#ifdef W_DEBUG
    virtual void print(std::ostream& stream) const {}
    friend std::ostream& operator <<(std::ostream& stream, const WEvent& event)
    {
        event.print(stream);
        return stream;
    }
#endif

    TimeStamp timestamp;
};

} // namespace erwin