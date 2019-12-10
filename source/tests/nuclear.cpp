#include <iostream>
#include <iomanip>
#include <vector>
#include <functional>
#include <random>
#include <type_traits>
#include <cstdlib>
#include <cstring>
#include <bitset>

#include "glm/glm.hpp"
#include "memory/memory.hpp"
#include "memory/pool_allocator.h"
#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "debug/logger_sink.h"

using namespace erwin;

void init_logger()
{
    WLOGGER.create_channel("memory", 3);
	WLOGGER.create_channel("nuclear", 3);
	WLOGGER.attach_all("console_sink", std::make_unique<dbg::ConsoleSink>());
    WLOGGER.set_single_threaded(true);
    WLOGGER.set_backtrace_on_error(false);
    WLOGGER.spawn();
    WLOGGER.sync();

    DLOGN("nuclear") << "Nuclear test" << std::endl;
}

typedef memory::MemoryArena<memory::PoolAllocator, 
		    				memory::policy::SingleThread, 
		    				memory::policy::SimpleBoundsChecking,
		    				memory::policy::NoMemoryTagging,
		    				memory::policy::SimpleMemoryTracking> PoolArena;

struct Transform
{
	uint32_t position;
	uint32_t angle;
};

int main(int argc, char** argv)
{
	init_logger();

    memory::hex_dump_highlight(0xf0f0f0f0, WCB(100,0,0));
    memory::hex_dump_highlight(0x0f0f0f0f, WCB(0,0,100));
    memory::hex_dump_highlight(0xd0d0d0d0, WCB(200,100,0));

    size_t max_count = 32;
    size_t decoration_size = PoolArena::DECORATION_SIZE;
    size_t decorated_size = sizeof(Transform) + decoration_size;
    size_t pool_size = max_count * decorated_size;

	memory::HeapArea area(1_MB);
	PoolArena tf_pool(area.begin(), sizeof(Transform), max_count, PoolArena::DECORATION_SIZE);

	std::vector<Transform*> transforms;
	for(int ii=0; ii<10; ++ii)
	{
		Transform* tf = W_NEW(Transform, tf_pool);
		tf->position = ii;
		tf->angle = ii+1;
		transforms.push_back(tf);
	}

    memory::hex_dump(std::cout, tf_pool.get_allocator().begin(), pool_size, "HEX DUMP");

    for(int ii=0; ii<5; ++ii)
    	W_DELETE(transforms[ii], tf_pool);

    memory::hex_dump(std::cout, tf_pool.get_allocator().begin(), pool_size, "HEX DUMP");

	for(int ii=0; ii<5; ++ii)
	{
		Transform* tf = W_NEW(Transform, tf_pool);
		tf->position = ii+10;
		tf->angle = ii+11;
		transforms.push_back(tf);
	}

    memory::hex_dump(std::cout, tf_pool.get_allocator().begin(), pool_size, "HEX DUMP");

    for(int ii=0; ii<10; ++ii)
    	W_DELETE(transforms[ii], tf_pool);

    memory::hex_dump(std::cout, tf_pool.get_allocator().begin(), pool_size, "HEX DUMP");

	return 0;
}
