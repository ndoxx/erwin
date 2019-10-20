#include <iostream>
#include <iomanip>
#include <vector>
#include <functional>
#include <random>
#include <type_traits>
#include <cstdlib>
#include <cstring>

#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "debug/logger_sink.h"
#include "memory/memory.hpp"
#include "memory/linear_allocator.h"
#include "memory/memory_utils.h"

using namespace erwin;

struct POD
{
	uint32_t a;
	uint32_t b;
	uint64_t plop;
};

struct NonPOD
{
	NonPOD(uint32_t a, uint32_t b):
	a(a), b(b), c(555)
	{
		data = new uint32_t[a];
		for(int ii=0; ii<a; ++ii)
			data[ii] = b;
	}

	NonPOD():NonPOD(4,2) { }

	~NonPOD()
	{
		delete[] data;
	}

	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t* data;
};

typedef memory::MemoryArena<memory::LinearAllocator, 
		    		memory::policy::SingleThread, 
		    		memory::policy::SimpleBoundsChecking,
		    		memory::policy::NoMemoryTagging,
		    		memory::policy::SimpleMemoryTracking> TestArena;

int main(int argc, char** argv)
{
    WLOGGER.create_channel("memory", 3);
    WLOGGER.attach_all("ConsoleSink", std::make_unique<dbg::ConsoleSink>());
    WLOGGER.set_single_threaded(true);

    memory::hex_dump_highlight(0xf0f0f0f0, WCB(100,0,0));
    memory::hex_dump_highlight(0x0f0f0f0f, WCB(0,0,100));
    memory::hex_dump_highlight(0xd0d0d0d0, WCB(200,100,0));

	memory::HeapArea area(1000);
	TestArena arena(area);

	std::cout << "POD is POD: " << std::is_pod<POD>::value << std::endl;
	std::cout << "NonPOD is POD: " << std::is_pod<NonPOD>::value << std::endl;
	std::cout << std::endl;
	
	{
		std::cout << "--- new POD non-aligned ---" << std::endl;
		POD* some_pod = W_NEW(POD, arena);
		some_pod->a = 42;
		some_pod->b = 56;
		some_pod->plop = 5587474657354873254;
		memory::hex_dump(reinterpret_cast<uint8_t*>(some_pod) - TestArena::BK_FRONT_SIZE, sizeof(POD) + TestArena::DECORATION_SIZE);
		W_DELETE(some_pod, arena);
		std::cout << std::endl;
	}

	{
		std::cout << "--- new POD aligned ---" << std::endl;
		POD* some_pod = W_NEW_ALIGN(POD, arena, 16);
		some_pod->a = 42;
		some_pod->b = 56;
		some_pod->plop = 5587474657354873254;
		memory::hex_dump(reinterpret_cast<uint8_t*>(some_pod) - TestArena::BK_FRONT_SIZE, sizeof(POD) + TestArena::DECORATION_SIZE);
		W_DELETE(some_pod, arena);
		std::cout << std::endl;
	}

	{
		std::cout << "--- new POD array non-aligned ---" << std::endl;
		POD* pod_array = W_NEW_ARRAY(POD[10], arena);
		for(int ii=0; ii<10; ++ii)
		{
			pod_array[ii].a = 42;
			pod_array[ii].b = 56;
		}
		memory::hex_dump(reinterpret_cast<uint8_t*>(pod_array) - TestArena::BK_FRONT_SIZE, 10*sizeof(POD) + TestArena::DECORATION_SIZE);
		W_DELETE_ARRAY(pod_array, arena);
		std::cout << std::endl;
	}

	{
		std::cout << "--- new POD array aligned ---" << std::endl;
		POD* pod_array = W_NEW_ARRAY_ALIGN(POD[10], arena, 16);
		for(int ii=0; ii<10; ++ii)
		{
			pod_array[ii].a = 42;
			pod_array[ii].b = 56;
		}
		memory::hex_dump(reinterpret_cast<uint8_t*>(pod_array) - TestArena::BK_FRONT_SIZE, 10*sizeof(POD) + TestArena::DECORATION_SIZE);
		W_DELETE_ARRAY(pod_array, arena);
		std::cout << std::endl;
	}

	{
		std::cout << "--- new non-POD non-aligned ---" << std::endl;
		NonPOD* some_non_pod = W_NEW(NonPOD, arena)(10,8);
		memory::hex_dump(reinterpret_cast<uint8_t*>(some_non_pod) - TestArena::BK_FRONT_SIZE, sizeof(NonPOD) + TestArena::DECORATION_SIZE);
		W_DELETE(some_non_pod, arena);
		std::cout << std::endl;
	}

	{
		std::cout << "--- new non-POD aligned ---" << std::endl;
		NonPOD* some_non_pod = W_NEW_ALIGN(NonPOD, arena, 16)(10,8);
		memory::hex_dump(reinterpret_cast<uint8_t*>(some_non_pod) - TestArena::BK_FRONT_SIZE, sizeof(NonPOD) + TestArena::DECORATION_SIZE);
		W_DELETE(some_non_pod, arena);
		std::cout << std::endl;
	}

	{
		std::cout << "--- new non-POD array non-aligned ---" << std::endl;
		NonPOD* non_pod_array = W_NEW_ARRAY(NonPOD[4], arena);
		memory::hex_dump(reinterpret_cast<uint8_t*>(non_pod_array) - TestArena::BK_FRONT_SIZE - 4, 4*sizeof(NonPOD) + TestArena::DECORATION_SIZE + 4);
		W_DELETE_ARRAY(non_pod_array, arena);
		std::cout << std::endl;
	}

	{
		std::cout << "--- new non-POD array aligned ---" << std::endl;
		NonPOD* non_pod_array = W_NEW_ARRAY_ALIGN(NonPOD[4], arena, 16);
		memory::hex_dump(reinterpret_cast<uint8_t*>(non_pod_array) - TestArena::BK_FRONT_SIZE - 4, 4*sizeof(NonPOD) + TestArena::DECORATION_SIZE + 4);
		W_DELETE_ARRAY(non_pod_array, arena);
		std::cout << std::endl;
	}

	std::cout << "--- full hex dump ---" << std::endl;
	memory::hex_dump(reinterpret_cast<uint8_t*>(area.begin()), 1000);

/*
	{
		std::cout << "--- back overwrite test ---" << std::endl;
		POD* some_pod = W_NEW(POD, arena);
		uint8_t* ptr = (uint8_t*)some_pod;
		std::fill(ptr, ptr + sizeof(POD) + 8, 0xAA);
		W_DELETE(some_pod, arena);
		std::cout << std::endl;
	}
*/
/*
	{
		std::cout << "--- front overwrite test ---" << std::endl;
		POD* some_pod = W_NEW(POD, arena);
		POD* some_other_pod = W_NEW(POD, arena);
		uint8_t* ptr = (uint8_t*)some_pod;
		std::fill(ptr, ptr + sizeof(POD) + 8, 0xAA);
		W_DELETE(some_other_pod, arena);
		W_DELETE(some_pod, arena);
		std::cout << std::endl;
	}
*/
	return 0;
}


/*
#include "render/render_queue.hpp"
struct InstancedSpriteQueueData
{
	hash_t shader;
	hash_t texture;
	uint32_t UBO;
	uint32_t SSBO;

	inline void reset()
	{
		shader = texture = SSBO = UBO = 0;
	}
};

template <>
struct SortKeyCreator<InstancedSpriteQueueData>
{
	inline uint64_t operator()(const InstancedSpriteQueueData& data)
	{
		return uint8_t(data.UBO)
		    + (uint8_t(data.texture) << 8)
		    + (uint8_t(data.shader) << 16);
	}
};

int main(int argc, char** argv)
{
	std::default_random_engine generator;
	std::uniform_int_distribution<uint32_t> dis10(0,9);

	RenderQueue<InstancedSpriteQueueData> isp_queue;
	isp_queue.resize_pool(8192,32);
	isp_queue.set_command_handlers(
	[&](const InstancedSpriteQueueData& data)
	{
		std::cout << "[" << data.shader << "," << data.texture 
				  << "," << data.UBO << "," << data.SSBO << "]" << std::endl;
	},
	[&](const PassState& state)
	{
		std::cout << "[STATE rt: " << state.render_target << "]" << std::endl;
	});

	for(int jj=0; jj<3; ++jj)
	{
		auto* state = isp_queue.pass_state_ptr();
		state->render_target = jj;
		isp_queue.begin_pass(state);

		for(int ii=0; ii<30; ++ii)
		{
			auto* data = isp_queue.data_ptr();
			data->shader  = uint64_t(ii%2);
			data->texture = dis10(generator)%3;
			data->UBO     = dis10(generator)%5;
			data->SSBO    = dis10(generator);
			isp_queue.push(data);
		}
	}

	isp_queue.flush();

	return 0;
}
*/