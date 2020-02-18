#include "event/event_bus.h"

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
    for(auto&& [key, delegates]: subscribers_)
    {
    	if(delegates)
        	for(auto&& handler: *delegates)
            	delete handler;
        delete delegates;
    }
}

void EventBus::dispatch()
{
	// TODO: Handlers may fire events too, maybe we should continue to iterate till all queues are truly empty
	for(auto&& [eid, q]: event_queues_)
	{
        DelegateList* delegates = subscribers_[eid];
        if(delegates == nullptr) continue;

        // TODO: Implement a timeout to distribute load across several frames
        while(!q.empty())
        {
	        for(auto&& handler: *delegates)
	        {        		
	            if(handler == nullptr) continue;
	            if(handler->exec(*q.front())) break; // If handler returns true, event is not propagated further
	        }
	        
	        delete q.front();
	        q.pop();
	    }
	}
}

} // namespace erwin