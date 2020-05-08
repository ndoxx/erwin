#include <array>
#include <atomic>
#include <bitset>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <queue>
#include <random>
#include <type_traits>
#include <vector>

#include "ctti/type_id.hpp"
#include "debug/logger.h"
#include "debug/logger_sink.h"
#include "debug/logger_thread.h"
#include "glm/glm.hpp"
#include "memory/memory.hpp"

using namespace erwin;

void init_logger()
{
    WLOGGER(create_channel("memory", 3));
    WLOGGER(create_channel("thread", 3));
    WLOGGER(create_channel("nuclear", 3));
    WLOGGER(create_channel("entity", 3));
    WLOGGER(create_channel("config", 3));
    WLOGGER(create_channel("render", 3));
    WLOGGER(attach_all("console_sink", std::make_unique<dbg::ConsoleSink>()));
    WLOGGER(set_single_threaded(true));
    WLOGGER(set_backtrace_on_error(false));
    WLOGGER(spawn());
    WLOGGER(sync());

    DLOGN("nuclear") << "Nuclear test" << std::endl;
}

namespace dev
{

template <typename EventT>
class AbstractDelegate
{
public:
    virtual ~AbstractDelegate() = default;
    inline bool exec(const EventT& event) { return call(event); }

private:
    virtual bool call(const EventT& event) = 0;
};

template <typename T, typename EventT>
class MemberDelegate : public AbstractDelegate<EventT>
{
public:
    virtual ~MemberDelegate() = default;
    typedef bool (T::*MemberFunction)(const EventT&);

    MemberDelegate(T* instance, MemberFunction memberFunction) : instance{instance}, memberFunction{memberFunction} {};

    // Cast event to the correct type and call member function
    virtual bool call(const EventT& event) override { return (instance->*memberFunction)(event); }

private:
    T* instance;                   // Pointer to class instance
    MemberFunction memberFunction; // Pointer to member function
};

template <typename EventT>
class FreeDelegate : public AbstractDelegate<EventT>
{
public:
    virtual ~FreeDelegate() = default;
    typedef bool (*FreeFunction)(const EventT&);

    explicit FreeDelegate(FreeFunction freeFunction) : freeFunction{freeFunction} {};

    // Cast event to the correct type and call member function
    virtual bool call(const EventT& event) override { return (*freeFunction)(event); }

private:
    FreeFunction freeFunction; // Pointer to member function
};

class AbstractEventQueue
{
public:
    virtual ~AbstractEventQueue() = default;
    virtual void process() = 0;
    virtual bool empty() const = 0;
    virtual size_t size() const = 0;
};

template <typename EventT> class EventQueue : public AbstractEventQueue
{
public:
    virtual ~EventQueue() = default;

    template <typename ClassT> inline void subscribe(ClassT* instance, bool (ClassT::*memberFunction)(const EventT&), size_t priority)
    {
        delegates_.push_back({priority, std::make_unique<MemberDelegate<ClassT, EventT>>(instance, memberFunction)});
        delegates_.sort(std::greater_equal<PriorityDelegate>());
    }

    inline void subscribe(bool (*freeFunction)(const EventT&), size_t priority)
    {
        delegates_.push_back({priority, std::make_unique<FreeDelegate<EventT>>(freeFunction)});
        delegates_.sort(std::greater_equal<PriorityDelegate>());
    }

    inline void submit(const EventT& event) { queue_.push(event); }

    inline void fire(const EventT& event)
    {
        for(auto&& [priority, handler]: delegates_)
            if(handler->exec(event))
                break;
    }

    virtual void process() override
    {
        while(!queue_.empty())
        {
            for(auto&& [priority, handler]: delegates_)
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

class EventBus
{
public:
    template <typename EventT>
    static void subscribe(bool (*freeFunction)(const EventT&), size_t priority=0u)
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
    static void subscribe(ClassT* instance, bool (ClassT::*memberFunction)(const EventT&), size_t priority=0u)
    {
        constexpr EventID k_id = ctti::type_id<EventT>().hash();
        auto& queue = event_queues_[k_id];
        if(queue == nullptr)
            queue = std::make_unique<EventQueue<EventT>>();

        auto* q_base_ptr = queue.get();
        auto* q_ptr = static_cast<EventQueue<EventT>*>(q_base_ptr);
        q_ptr->subscribe(instance, memberFunction, priority);
    }

    template <typename EventT>
    static void publish(const EventT& event)
    {
        constexpr EventID k_id = ctti::type_id<EventT>().hash();
        auto& queue = event_queues_[k_id];
        if(queue == nullptr)
            return;

        auto* q_base_ptr = queue.get();
        auto* q_ptr = static_cast<EventQueue<EventT>*>(q_base_ptr);
        q_ptr->fire(event);
    }

    template <typename EventT>
    static void enqueue(const EventT& event)
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

EventBus::EventQueues EventBus::event_queues_;

} // namespace dev

struct PlopEvent
{
    int a;
    int b;
};

bool free_plop_0(const PlopEvent& event)
{
    DLOGW("nuclear") << "[free_plop_0] Plop! " << event.a << " " << event.b << std::endl;
    return false;
}

bool free_plop_1(const PlopEvent& event)
{
    DLOGW("nuclear") << "[free_plop_1] Plop! " << event.a << " " << event.b << std::endl;
    return false;
}

bool free_plop_2(const PlopEvent& event)
{
    DLOGW("nuclear") << "[free_plop_2] Plop! " << event.a << " " << event.b << std::endl;
    return false;
}

class PlopSubber
{
public:
    PlopSubber() { dev::EventBus::subscribe(this, &PlopSubber::on_plop); }

private:
    bool on_plop(const PlopEvent& event)
    {
        DLOGW("nuclear") << "[PlopSubber] Plop! " << event.a << " " << event.b << std::endl;
        return false;
    }
};

int main(int argc, char** argv)
{
    init_logger();

    dev::EventBus::subscribe(free_plop_0,10);
    dev::EventBus::subscribe(free_plop_1);

    dev::EventBus::publish(PlopEvent{12, 24});

    PlopSubber ps;
    dev::EventBus::subscribe(free_plop_2);

    for(int ii = 0; ii < 4; ++ii)
        dev::EventBus::enqueue(PlopEvent{ii, -ii});

    DLOG("nuclear",1) << "Unprocessed: " << dev::EventBus::get_unprocessed_count() << std::endl;

    dev::EventBus::dispatch();
    DLOG("nuclear",1) << "Unprocessed: " << dev::EventBus::get_unprocessed_count() << std::endl;

    return 0;
}