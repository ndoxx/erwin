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
#include "core/handle.h"
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

ROBUST_HANDLE_DECLARATION(EntityHandle);
ROBUST_HANDLE_DEFINITION(EntityHandle, 128);

#define SHOW( ENT_HND ) DLOG("nuclear",1) << "(" << ENT_HND.index << "," << ENT_HND.counter << ")" << std::endl

int main(int argc, char** argv)
{
	init_logger();

	memory::HeapArea area(800_kB);
	LinearArena arena(area, 512_kB, "LinArena");
	EntityHandle::init_pool(arena);

	EntityHandle aa = EntityHandle::acquire();
	EntityHandle bb = EntityHandle::acquire();
	EntityHandle cc = EntityHandle::acquire();
	EntityHandle dd = EntityHandle::acquire();

	SHOW(aa);
	SHOW(bb);
	SHOW(cc);
	SHOW(dd) << std::endl;

	aa.release();
	bb.release();
	EntityHandle ee = EntityHandle::acquire();
	EntityHandle ff = EntityHandle::acquire();
	SHOW(ee);
	SHOW(ff);

	ee.release();
	EntityHandle g = EntityHandle::acquire();
	SHOW(g) << std::endl;

	dd.release();
	EntityHandle y = EntityHandle::acquire();
	SHOW(y);
	y.release();
	EntityHandle x = EntityHandle::acquire();
	SHOW(x);
	x.release();
	EntityHandle w = EntityHandle::acquire();
	SHOW(w) << std::endl;

	ff.release();
	EntityHandle v = EntityHandle::acquire();
	SHOW(v);

	EntityHandle::destroy_pool(arena);

	return 0;
}
