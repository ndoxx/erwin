// Inspired by https://medium.com/@savas/nomad-game-engine-part-7-the-event-system-45a809ccb68f
// * Using const references instead of pointers to pass events around
// * Using constexpr ctti::type_id<>() instead of RTTI
// * Leak free
// * Priority system

#pragma once
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <queue>

#include "ctti/type_id.hpp"

#include "core/core.h"
#include "event/delegate.h"

namespace erwin
{

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
    inline void subscribe(ClassT* instance, bool (ClassT::*memberFunction)(const EventT&), size_t priority)
    {
        delegates_.push_back({priority, std::make_unique<MemberDelegate<ClassT, EventT>>(instance, memberFunction)});
        delegates_.sort(std::greater_equal<PriorityDelegate>());
    }

    inline void subscribe(bool (*freeFunction)(const EventT&), size_t priority)
    {
        delegates_.push_back({priority, std::make_unique<FreeDelegate<EventT>>(freeFunction)});
        delegates_.sort(std::greater_equal<PriorityDelegate>());
    }

    // TODO: Event pooling if deemed necessary
    inline void submit(const EventT& event) { queue_.push(event); }

    inline void fire(const EventT& event)
    {
        for(auto&& [priority, handler] : delegates_)
            if(handler->exec(event))
                break;
    }

    virtual void process() override
    {
        while(!queue_.empty())
        {
            for(auto&& [priority, handler] : delegates_)
                if(handler->exec(queue_.front()))
                    break; // If handler returns true, event is not propagated further

            queue_.pop();
        }
    }

    virtual bool empty() const override { return queue_.empty(); }

    virtual size_t size() const override { return queue_.size(); }

private:
    using PriorityDelegate = std::pair<size_t, std::unique_ptr<AbstractDelegate<EventT>>>;
    using DelegateList = std::list<PriorityDelegate>;
    using Queue = std::queue<EventT>;
    DelegateList delegates_;
    Queue queue_;
};

// Central message broker
class EventBus
{
public:
    template <typename EventT> static void subscribe(bool (*freeFunction)(const EventT&), size_t priority = 0u)
    {
        constexpr EventID k_id = ctti::type_id<EventT>().hash();
        auto& queue = event_queues_[k_id];
        if(queue == nullptr)
            queue = std::make_unique<EventQueue<EventT>>();

        auto* q_base_ptr = queue.get();
        auto* q_ptr = static_cast<EventQueue<EventT>*>(q_base_ptr);
        q_ptr->subscribe(freeFunction, priority);
    }

    template <typename ClassT, typename EventT>
    static void subscribe(ClassT* instance, bool (ClassT::*memberFunction)(const EventT&), size_t priority = 0u)
    {
        constexpr EventID k_id = ctti::type_id<EventT>().hash();
        auto& queue = event_queues_[k_id];
        if(queue == nullptr)
            queue = std::make_unique<EventQueue<EventT>>();

        auto* q_base_ptr = queue.get();
        auto* q_ptr = static_cast<EventQueue<EventT>*>(q_base_ptr);
        q_ptr->subscribe(instance, memberFunction, priority);
    }

    template <typename EventT> static void publish(const EventT& event)
    {
        constexpr EventID k_id = ctti::type_id<EventT>().hash();
        auto& queue = event_queues_[k_id];
        if(queue == nullptr)
            return;

        auto* q_base_ptr = queue.get();
        auto* q_ptr = static_cast<EventQueue<EventT>*>(q_base_ptr);
        q_ptr->fire(event);
    }

    template <typename EventT> static void enqueue(const EventT& event)
    {
        constexpr EventID k_id = ctti::type_id<EventT>().hash();
        auto& queue = event_queues_[k_id];
        if(queue == nullptr)
            return;

        auto* q_base_ptr = queue.get();
        auto* q_ptr = static_cast<EventQueue<EventT>*>(q_base_ptr);
        q_ptr->submit(event);
    }

    static inline void dispatch()
    {
        for(auto&& [id, queue] : event_queues_)
            queue->process();
    }

    static inline bool empty()
    {
        for(auto&& [id, queue] : event_queues_)
            if(!queue->empty())
                return false;
        return true;
    }

    static inline size_t get_unprocessed_count()
    {
        return std::accumulate(event_queues_.begin(), event_queues_.end(), 0u,
                               [](size_t accumulator, auto&& entry) { return accumulator + entry.second->size(); });
    }

private:
    using EventID = uint64_t;
    using EventQueues = std::map<EventID, std::unique_ptr<AbstractEventQueue>>;
    static EventQueues event_queues_;
};

} // namespace erwin
