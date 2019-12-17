// Adapted from https://medium.com/@savas/nomad-game-engine-part-7-the-event-system-45a809ccb68f
// * Renamed some stuff to fit my conventions
// * Using const references instead of pointers to pass events around
// * Using constexpr ctti::type_id<>() instead of RTTI
// * Leak free
// * Made EventBus a (thread-safe) singleton (not something I'm really proud of)

#pragma once
#include <memory>
#include "EASTL/list.h"
#include "EASTL/hash_map.h"
#include "EASTL/queue.h"

#include "ctti/type_id.hpp"

#include "core/core.h"
#include "event/delegate.h"
#include "event/event.h"

namespace erwin
{

// Central message broker
class EventBus
{
public:
    NON_COPYABLE(EventBus);
    NON_MOVABLE(EventBus);

    using DelegateList = eastl::list<AbstractDelegate<WEvent>*>;

    static void init();
    static void shutdown();
    static inline EventBus& instance() { return *p_instance_; }

    EventBus() = default;
    ~EventBus();

    template<typename EventT>
    void init_event_pool(memory::HeapArea& area, size_t max_events)
    {
        EventT::init_pool(area.require_pool_block<PoolArena>(sizeof(EventT), max_events), max_events);
        event_queues_.emplace(eastl::piecewise_construct, eastl::make_tuple(EventT::ID), eastl::make_tuple());
    }

    template<typename EventT>
    void destroy_event_pool()
    {
        EventT::destroy_pool();
    }

    // Fire an event that will be handled instantly
    template<typename EventT>
    void publish(const EventT& event)
    {
        DelegateList* delegates = subscribers_[EventT::ID];
        if(delegates == nullptr) return;

        for(auto&& handler: *delegates)
            if(handler != nullptr)
                if(handler->exec(event)) // If handler returns true, event is not propagated further
                    break;
    }

    // Fire an event that will be processed during the event phase
    template<typename EventT>
    void enqueue(EventT* event)
    {
        DelegateList* delegates = subscribers_[EventT::ID];
        if(delegates == nullptr) return;

        event_queues_[EventT::ID].push(event);
    }

    // Dispatch all queued events to corresponding delegates
    void dispatch();

    // Register a member function as an event handler
    template<typename ClassT, typename EventT>
    void subscribe(ClassT* instance, bool (ClassT::*memberFunction)(const EventT&))
    {
        DelegateList* delegates = subscribers_[EventT::ID];

        // First time initialization
        if(delegates == nullptr)
        {
            delegates = new DelegateList();
            subscribers_[EventT::ID] = delegates;
        }

        delegates->push_back(new MemberDelegate<ClassT, WEvent, EventT>(instance, memberFunction));
    }

    // Register a free function as an event handler
    template<typename EventT>
    void subscribe(bool (*freeFunction)(const EventT&))
    {
        DelegateList* delegates = subscribers_[EventT::ID];

        // First time initialization
        if(delegates == nullptr)
        {
            delegates = new DelegateList();
            subscribers_[EventT::ID] = delegates;
        }

        delegates->push_back(new FreeDelegate<WEvent, EventT>(freeFunction));
    }

private:
    using Subscribers = eastl::hash_map<EventID, DelegateList*>;
    using EventQueues = eastl::hash_map<EventID, eastl::queue<WEvent*>>;

    Subscribers subscribers_;
    EventQueues event_queues_;

    static std::unique_ptr<EventBus> p_instance_;
};

#define EVENTBUS EventBus::instance()

} // namespace erwin
