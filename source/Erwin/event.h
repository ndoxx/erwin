// Adapted from https://medium.com/@savas/nomad-game-engine-part-7-the-event-system-45a809ccb68f
// * Renamed some stuff to fit my conventions
// * Using const references instead of pointers to pass events around
// * Using constexpr ctti::type_id<>() instead of RTTI
// * Leak free
// * Made EventBus a (thread-safe) singleton (not something I'm really proud of)

#pragma once
#include <map>
#include <list>
#include <ostream>

#include "ctti/type_id.hpp"

#include "time_base.h"
#include "singleton.hpp"

namespace erwin
{

// Base class for an event
struct WEvent
{
    WEvent(): timestamp(TimeBase::timestamp()) {}

#ifdef __DEBUG__
    virtual const char* get_name() const { return ""; }
    virtual void print(std::ostream& stream) const {}
    friend std::ostream& operator <<(std::ostream& stream, const WEvent& event)
    {
        event.print(stream);
        return stream;
    }
#endif

    TimeStamp timestamp;
};

// Interface for function wrappers that the event bus can register
class AbstractDelegate
{
public:
    virtual ~AbstractDelegate() = default;
    inline void exec(const WEvent& event) { call(event); }

private:
    virtual void call(const WEvent& event) = 0;
};

// Member function wrapper, to allow classes to register their member functions as event handlers
template<class T, class EventT>
class MemberDelegate: public AbstractDelegate
{
public:
    virtual ~MemberDelegate() = default;
    typedef void (T::*MemberFunction)(const EventT&);

    MemberDelegate(T* instance, MemberFunction memberFunction): instance{ instance }, memberFunction{ memberFunction } {};

    // Cast event to the correct type and call member function
    inline void call(const WEvent& event)
    {
        (instance->*memberFunction)(static_cast<const EventT&>(event));
    }

private:
    T* instance; // Pointer to class instance
    MemberFunction memberFunction; // Pointer to member function
};

// Central message broker
class EventBus: public Singleton<EventBus>
{
public:
    typedef std::list<AbstractDelegate*> DelegateList;

    friend EventBus& Singleton<EventBus>::Instance();
    friend void Singleton<EventBus>::Kill();

    // Send an event
    template<typename EventT>
    void publish(const EventT& event)
    {
        DelegateList* delegates = subscribers[ctti::type_id<EventT>()];

        if(delegates == nullptr) return;

        for(auto&& handler: *delegates)
            if(handler != nullptr)
                handler->exec(event);
    }

    // Register a member function as an event handler
    template<class T, class EventT>
    void subscribe(T* instance, void (T::*memberFunction)(const EventT&))
    {
        DelegateList* delegates = subscribers[ctti::type_id<EventT>()];

        // First time initialization
        if(delegates == nullptr)
        {
            delegates = new DelegateList();
            subscribers[ctti::type_id<EventT>()] = delegates;
        }

        delegates->push_back(new MemberDelegate<T, EventT>(instance, memberFunction));
    }

private:
    EventBus(const EventBus&)=delete;
    EventBus()=default;
    ~EventBus()
    {
        for(auto&& [key, delegates]: subscribers)
        {
            for(auto&& handler: *delegates)
                delete handler;
            delete delegates;
        }
    }

private:
    std::unordered_map<ctti::unnamed_type_id_t, DelegateList*> subscribers;
};

#define EVENTBUS EventBus::Instance()

} // namespace erwin
