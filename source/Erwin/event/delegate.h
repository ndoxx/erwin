#pragma once

#include <ostream>
#include <string>
#include <type_traits>
#include <functional>
#include "core/time_base.h"

namespace erwin
{

// Interface for function wrappers that an event bus can register
template <typename EventT>
class AbstractDelegate
{
public:
    virtual ~AbstractDelegate() = default;
    inline bool exec(const EventT& event) const { return call(event); }

private:
    virtual bool call(const EventT& event) const = 0;
};

// Member function wrapper, to allow classes to register their member functions as event handlers
template <typename T, typename EventT>
class MemberDelegate : public AbstractDelegate<EventT>
{
public:
    virtual ~MemberDelegate() = default;
    typedef bool (T::*MemberFunction)(const EventT&);

    MemberDelegate(T* instance, MemberFunction memberFunction) : instance{instance}, memberFunction{memberFunction} {};

    // Cast event to the correct type and call member function
    virtual bool call(const EventT& event) const override { return (instance->*memberFunction)(event); }

private:
    T* instance;                   // Pointer to class instance
    MemberFunction memberFunction; // Pointer to member function
};

// Free function wrapper
template <typename EventT>
class FreeDelegate : public AbstractDelegate<EventT>
{
public:
    virtual ~FreeDelegate() = default;
    typedef bool (*FreeFunction)(const EventT&);

    explicit FreeDelegate(FreeFunction freeFunction) : freeFunction{freeFunction} {};

    // Cast event to the correct type and call member function
    virtual bool call(const EventT& event) const override { return (*freeFunction)(event); }

private:
    FreeFunction freeFunction; // Pointer to member function
};

} // namespace erwin