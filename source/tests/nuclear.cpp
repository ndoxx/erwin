#include <iostream>
#include <iomanip>
#include <vector>
#include <functional>
#include <random>
#include <type_traits>
#include <cstdlib>
#include <cstring>
#include <bitset>
#include <atomic>

#include "glm/glm.hpp"
#include "memory/memory.hpp"
#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "debug/logger_sink.h"


#include "entity/wip/ecs.h"

using namespace erwin;

void init_logger()
{
    WLOGGER(create_channel("memory", 3));
    WLOGGER(create_channel("thread", 3));
	WLOGGER(create_channel("nuclear", 3));
	WLOGGER(create_channel("entity", 3));
	WLOGGER(create_channel("config", 3));
	WLOGGER(attach_all("console_sink", std::make_unique<dbg::ConsoleSink>()));
    WLOGGER(set_single_threaded(true));
    WLOGGER(set_backtrace_on_error(false));
    WLOGGER(spawn());
    WLOGGER(sync());

    DLOGN("nuclear") << "Nuclear test" << std::endl;
}

#define SHOW( ENT_HND ) DLOG("nuclear",1) << "(" << ENT_HND.index << "," << ENT_HND.counter << ")" << std::endl

using namespace wip;

class AComponent: public Component
{
public:
	COMPONENT_DECLARATION(AComponent);

	int a;
};
COMPONENT_DEFINITION(AComponent);

int main(int argc, char** argv)
{
	init_logger();

	memory::HeapArea area(1_MB);

	ECS::init(area);
	ECS::create_component_storage<AComponent>(area, 128);


	EntityHandle aa = ECS::create_entity();
	EntityHandle bb = ECS::create_entity();
	EntityHandle cc = ECS::create_entity();
	EntityHandle dd = ECS::create_entity();

	SHOW(aa);
	SHOW(bb);
	SHOW(cc);
	SHOW(dd) << std::endl;

	DLOG("nuclear",1) << aa.is_valid() << std::endl;

	ECS::destroy_entity(aa);
	DLOG("nuclear",1) << aa.is_valid() << std::endl;

	ECS::destroy_entity(bb);
	EntityHandle ee = ECS::create_entity();
	EntityHandle ff = ECS::create_entity();
	SHOW(ee);
	SHOW(ff);

	ECS::destroy_entity(ee);
	EntityHandle g = ECS::create_entity();
	SHOW(g) << std::endl;

	ECS::destroy_entity(dd);
	EntityHandle y = ECS::create_entity();
	SHOW(y);
	ECS::destroy_entity(y);
	EntityHandle x = ECS::create_entity();
	SHOW(x);
	ECS::destroy_entity(x);
	EntityHandle w = ECS::create_entity();
	SHOW(w) << std::endl;

	DLOG("nuclear",1) << ff.is_valid() << std::endl;
	ECS::destroy_entity(ff);
	EntityHandle v = ECS::create_entity();
	SHOW(v);

	DLOG("nuclear",1) << aa.is_valid() << std::endl;
	DLOG("nuclear",1) << ff.is_valid() << std::endl;
	DLOG("nuclear",1) << v.is_valid() << std::endl;


	ECS::shutdown();

	return 0;
}
