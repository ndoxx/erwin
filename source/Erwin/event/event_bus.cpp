#include "event/event_bus.h"

namespace erwin
{

EventBus::EventQueues EventBus::event_queues_;
#ifdef W_DEBUG
std::map<EventID, bool> EventBus::event_filter_;
#endif

} // namespace erwin