// Adapted from https://medium.com/@savas/nomad-game-engine-part-7-the-event-system-45a809ccb68f
// * Renamed some stuff to fit my conventions
// * Using const references instead of pointers to pass events around
// * Using constexpr ctti::type_id<>() instead of RTTI
// * Leak free
// * Made EventBus a (thread-safe) singleton (not something I'm really proud of)

#pragma once
#include <map>
#include <list>
#include <memory>

#include "ctti/type_id.hpp"

#include "core/core.h"
#include "event/delegate.h"
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
        W_ASSERT(s_ppool_==nullptr, "Memory pool is already initialized."); \
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
        W_ASSERT(s_ppool_, "Memory pool has not been created yet. Call init_pool()."); \
        return ::W_NEW( EVENT_NAME , (*s_ppool_) ); \
    } \
    void EVENT_NAME::operator delete(void* ptr) \
    { \
        W_ASSERT(s_ppool_, "Memory pool has not been created yet. Call init_pool()."); \
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

// Central message broker
class EventBus
{
public:
    NON_COPYABLE(EventBus);
    NON_MOVABLE(EventBus);

    typedef std::list<AbstractDelegate<WEvent>*> DelegateList;

    static void init();
    static void shutdown();
    static inline EventBus& instance() { return *p_instance_; }

    EventBus() = default;
    ~EventBus();

    // Fire an event that will be handled instantly
    template<typename EventT>
    void publish(const EventT& event)
    {
        DelegateList* delegates = subscribers[EventT::ID];

        if(delegates == nullptr) return;

        // TODO: make it non-blocking (events handled in an event phase)
        for(auto&& handler: *delegates)
            if(handler != nullptr)
                if(handler->exec(event)) // If handler returns true, event is not propagated further
                    break;
    }

    // Fire an event that will be processed during the event phase
    template<typename EventT>
    void enqueue(const EventT& event)
    {

    }

    // Register a member function as an event handler
    template<typename ClassT, typename EventT>
    void subscribe(ClassT* instance, bool (ClassT::*memberFunction)(const EventT&))
    {
        DelegateList* delegates = subscribers[EventT::ID];

        // First time initialization
        if(delegates == nullptr)
        {
            delegates = new DelegateList();
            subscribers[EventT::ID] = delegates;
        }

        delegates->push_back(new MemberDelegate<ClassT, WEvent, EventT>(instance, memberFunction));
    }

    // Register a free function as an event handler
    template<typename EventT>
    void subscribe(bool (*freeFunction)(const EventT&))
    {
        DelegateList* delegates = subscribers[EventT::ID];

        // First time initialization
        if(delegates == nullptr)
        {
            delegates = new DelegateList();
            subscribers[EventT::ID] = delegates;
        }

        delegates->push_back(new FreeDelegate<WEvent, EventT>(freeFunction));
    }

private:
    using Subscribers = std::map<EventID, DelegateList*>;
    // using EventQueues = std::map<EventID, std::>;

    Subscribers subscribers;

    static std::unique_ptr<EventBus> p_instance_;
};

#define EVENTBUS EventBus::instance()

} // namespace erwin
