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

#include "event/event_bus.h"

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



int main(int argc, char** argv)
{
    init_logger();

    SubscriberPriorityKey p0(1,0,0);
    DLOG("nuclear",1) << int(p0.flags) << " " << int(p0.layer_id) << " " << int(p0.system_id) << std::endl;
    DLOG("nuclear",1) << p0.encode() << std::endl;
    DLOG("nuclear",1) << std::bitset<32>(p0.encode()) << std::endl;

    auto key = subscriber_priority(1,1,0);
    DLOG("nuclear",1) << key << std::endl;
    DLOG("nuclear",1) << std::bitset<32>(key) << std::endl;
    
    return 0;
}