#pragma once

#include <ostream>
#include <string>
#include <type_traits>
#include "core/time_base.h"

namespace erwin
{

// Interface for function wrappers that an event bus can register
template <typename BaseEventT>
class AbstractDelegate
{
public:
    virtual ~AbstractDelegate() = default;
    inline bool exec(const BaseEventT& event) { return call(event); }

private:
    virtual bool call(const BaseEventT& event) = 0;
};

// Member function wrapper, to allow classes to register their member functions as event handlers
template<typename T, typename BaseEventT, typename EventT,
		 typename = typename std::enable_if<std::is_base_of<BaseEventT, EventT>::value, EventT>::type>
class MemberDelegate: public AbstractDelegate<BaseEventT>
{
public:
    virtual ~MemberDelegate() = default;
    typedef bool (T::*MemberFunction)(const EventT&);

    MemberDelegate(T* instance, MemberFunction memberFunction): instance{ instance }, memberFunction{ memberFunction } {};

    // Cast event to the correct type and call member function
    virtual bool call(const BaseEventT& event) override
    {
        return (instance->*memberFunction)(static_cast<const EventT&>(event));
    }

private:
    T* instance; // Pointer to class instance
    MemberFunction memberFunction; // Pointer to member function
};

// Free function wrapper
template<typename BaseEventT, typename EventT,
		 typename = typename std::enable_if<std::is_base_of<BaseEventT, EventT>::value, EventT>::type>
class FreeDelegate: public AbstractDelegate<BaseEventT>
{
public:
    virtual ~FreeDelegate() = default;
    typedef bool (*FreeFunction)(const EventT&);

    explicit FreeDelegate(FreeFunction freeFunction): freeFunction{ freeFunction } {};

    // Cast event to the correct type and call member function
    virtual bool call(const BaseEventT& event) override
    {
        return (*freeFunction)(static_cast<const EventT&>(event));
    }

private:
    FreeFunction freeFunction; // Pointer to member function
};

} // namespace erwin