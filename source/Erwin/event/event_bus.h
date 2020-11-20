// Inspired by https://medium.com/@savas/nomad-game-engine-part-7-the-event-system-45a809ccb68f
// Differences are:
// * Using const references or rvalues instead of pointers to pass events around
// * Using constexpr ctti::type_id<>() instead of RTTI
// * Leak free
// * Events can be any type, no need to derive from a base event class
// * Deferred event handling with event queues, instant firing still supported
// * Priority mechanics

// TODO:
// [ ] Dispatch timeout
// [ ] Dispatch deadlock detection?
// [ ] Event pooling if deemed necessary

#pragma once
#include <vector>
#include <map>
#include <memory>
#include <numeric>
#include <queue>

#include "core/config.h"
#include "core/core.h"
#include <kibble/logger/logger.h>
#include "event/delegate.h"
#include "event/event.h"

namespace erwin
{

struct SubscriberPriorityKey
{
    uint16_t flags;
    uint8_t layer_id;
    uint8_t system_id;

    static constexpr uint32_t k_flags_shift = 32u - 16u;
    static constexpr uint32_t k_layer_shift = k_flags_shift - 8u;
    static constexpr uint32_t k_system_shift = k_layer_shift - 8u;
    static constexpr uint32_t k_flags_mask = uint32_t(0x0000ffff) << k_flags_shift;
    static constexpr uint32_t k_layer_mask = uint32_t(0x000000ff) << k_layer_shift;
    static constexpr uint32_t k_system_mask = uint32_t(0x000000ff) << k_system_shift;

    SubscriberPriorityKey() : flags(0), layer_id(0), system_id(0) {}

    SubscriberPriorityKey(uint8_t _layer_id, uint8_t _system_id = 0, uint16_t _flags = 0)
        : flags(_flags), layer_id(_layer_id), system_id(_system_id)
    {}

    inline uint32_t encode()
    {
        return (uint32_t(flags) << k_flags_shift) | (uint32_t(layer_id) << k_layer_shift) |
               (uint32_t(system_id) << k_system_shift);
    }

    inline void decode(uint32_t priority)
    {
        flags = uint16_t((priority & k_flags_mask) >> k_flags_shift);
        layer_id = uint8_t((priority & k_layer_mask) >> k_layer_shift);
        system_id = uint8_t((priority & k_system_mask) >> k_system_shift);
    }
};

[[maybe_unused]] static inline uint32_t subscriber_priority(uint8_t layer_id, uint8_t system_id = 0u,
                                                            uint16_t flags = 0u)
{
    return SubscriberPriorityKey(layer_id, system_id, flags).encode();
}

// Interface for an event queue
class AbstractEventQueue
{
public:
    virtual ~AbstractEventQueue() = default;
    virtual void process() = 0;
    virtual bool empty() const = 0;
    virtual size_t size() const = 0;
};

// Concrete event queue, can subscribe functions, and process events immediately or in a deferred fashion
template <typename EventT> class EventQueue : public AbstractEventQueue
{
public:
    virtual ~EventQueue() = default;

    template <typename ClassT>
    inline void subscribe(ClassT* instance, bool (ClassT::*memberFunction)(const EventT&), uint32_t priority)
    {
        delegates_.push_back({priority, std::make_unique<MemberDelegate<ClassT, EventT>>(instance, memberFunction)});
        // greater_equal generates the desired behavior: for equal priority range of subscribers,
        // latest subscriber handles the event first
        std::sort(delegates_.begin(), delegates_.end(), std::greater_equal<PriorityDelegate>());
    }

    inline void subscribe(bool (*freeFunction)(const EventT&), uint32_t priority)
    {
        delegates_.push_back({priority, std::make_unique<FreeDelegate<EventT>>(freeFunction)});
        std::sort(delegates_.begin(), delegates_.end(), std::greater_equal<PriorityDelegate>());
    }

    inline void submit(const EventT& event) { queue_.push(event); }
    inline void submit(EventT&& event) { queue_.push(event); }

    inline void fire(const EventT& event) const
    {
        for(auto&& [priority, handler] : delegates_)
            if(handler->exec(event))
                break; // If handler returns true, event is not propagated further
    }

    virtual void process() override
    {
        while(!queue_.empty())
        {
            for(auto&& [priority, handler] : delegates_)
                if(handler->exec(queue_.front()))
                    break;

            queue_.pop();
        }
    }

    virtual bool empty() const override { return queue_.empty(); }

    virtual size_t size() const override { return queue_.size(); }

private:
    using PriorityDelegate = std::pair<uint32_t, std::unique_ptr<AbstractDelegate<EventT>>>;
    using DelegateList = std::vector<PriorityDelegate>;
    using Queue = std::queue<EventT>;
    DelegateList delegates_;
    Queue queue_;
};

// Central message broker
class EventBus
{
public:
    // Subscribe a free function to a particular event type
    template <typename EventT> static void subscribe(bool (*freeFunction)(const EventT&), uint32_t priority = 0u)
    {
        auto* q_base_ptr = get_or_create<EventT>().get();
        auto* q_ptr = static_cast<EventQueue<EventT>*>(q_base_ptr);
        q_ptr->subscribe(freeFunction, priority);
    }

    // Subscribe a member function to a particular event type
    template <typename ClassT, typename EventT>
    static void subscribe(ClassT* instance, bool (ClassT::*memberFunction)(const EventT&), uint32_t priority = 0u)
    {
        auto* q_base_ptr = get_or_create<EventT>().get();
        auto* q_ptr = static_cast<EventQueue<EventT>*>(q_base_ptr);
        q_ptr->subscribe(instance, memberFunction, priority);
    }

    // Fire an event and have it handled immediately
    template <typename EventT> static void fire(const EventT& event)
    {
        try_get<EventT>([&event](auto* q_ptr) {
#ifdef W_DEBUG
            log_event(event);
#endif
            q_ptr->fire(event);
        });
    }

    // Enqueue an event for deferred handling (during the dispatch() call)
    template <typename EventT> static void enqueue(const EventT& event)
    {
        try_get<EventT>([&event](auto* q_ptr) {
#ifdef W_DEBUG
            log_event(event);
#endif
            q_ptr->submit(event);
        });
    }

    template <typename EventT> static void enqueue(EventT&& event)
    {
        try_get<EventT>([&event](auto* q_ptr) {
#ifdef W_DEBUG
            log_event(event);
#endif
            q_ptr->submit(std::forward<EventT>(event));
        });
    }

    // Handle all queued events
    static inline void dispatch()
    {
        // An event once handled can cause another event to be enqueued, so
        // we iterate till all events have been processed
        while(!empty())
            for(auto&& [id, queue] : event_queues_)
                queue->process();
    }

    // Check if all queues are empty
    static inline bool empty()
    {
        for(auto&& [id, queue] : event_queues_)
            if(!queue->empty())
                return false;

        return true;
    }

    // Get the number of unprocessed events
    static inline size_t get_unprocessed_count()
    {
        return std::accumulate(event_queues_.begin(), event_queues_.end(), 0u, [](size_t accumulator, auto&& entry) {
            return accumulator + (entry.second ? entry.second->size() : 0u);
        });
    }

#ifdef W_DEBUG
    // Enable event tracking for a particular type of events
    template <typename EventT> static inline void track_event(bool value = true) { event_filter_[EventT::ID] = value; }

    // Lookup config and enable/disable event tracking for this particular event type
    template <typename EventT> static inline void configure_event_tracking()
    {
        std::string config_key_str = "erwin.events.track." + std::string(EventT::NAME);
        track_event<EventT>(cfg::get<bool>(H_(config_key_str.c_str()), false));
    }

    // Log an event
    template <typename EventT> static inline void log_event(const EventT& event)
    {
        if(event_filter_[EventT::ID])
        {
            kb::klog::get_log("event"_h, kb::klog::MsgType::EVENT, 0)
                << "\033[1;38;2;0;0;0m\033[1;48;2;0;185;153m[" << event.NAME << "]\033[0m " << event << std::endl;
        }
    }
#endif

#ifdef W_TEST
    // Clear all subscribers. Only enabled for unit testing.
    static inline void reset() { event_queues_.clear(); }
#endif

private:
    // Helper function to get a particular event queue if it exists or create a new one if not
    template <typename EventT> static auto& get_or_create()
    {
        auto& queue = event_queues_[EventT::ID];
        if(queue == nullptr)
        {
            queue = std::make_unique<EventQueue<EventT>>();
#ifdef W_DEBUG
            configure_event_tracking<EventT>();
#endif
        }
        return queue;
    }

    // Helper function to access a queue only if it exists
    template <typename EventT> static inline void try_get(std::function<void(EventQueue<EventT>*)> visit)
    {
        auto findit = event_queues_.find(EventT::ID);
        if(findit != event_queues_.end())
        {
            auto* q_base_ptr = findit->second.get();
            auto* q_ptr = static_cast<EventQueue<EventT>*>(q_base_ptr);
            visit(q_ptr);
        }
    }

private:
    using EventQueues = std::map<EventID, std::unique_ptr<AbstractEventQueue>>;
    static EventQueues event_queues_;
#ifdef W_DEBUG
    static std::map<EventID, bool> event_filter_; // Controls which tracked events are sent to the logger
#endif
};

} // namespace erwin
