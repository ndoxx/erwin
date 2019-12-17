#include "event/event.h"

namespace erwin
{

std::unique_ptr<EventBus> EventBus::p_instance_ = nullptr;

void EventBus::init()
{
	p_instance_ = std::make_unique<EventBus>();
}

void EventBus::shutdown()
{
	p_instance_ = nullptr;
}

EventBus::~EventBus()
{
    for(auto&& [key, delegates]: subscribers)
    {
        for(auto&& handler: *delegates)
            delete handler;
        delete delegates;
    }
}

} // namespace erwin