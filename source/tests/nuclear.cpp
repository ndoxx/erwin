#include <iostream>
#include <vector>
#include <functional>
#include <random>
#include <type_traits>
#include <cstdlib>
#include <cstring>

#include "memory/memory.hpp"
#include "memory/linear_allocator.h"

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
		std::cout << "NonPOD ctor" << std::endl;
		data = new uint32_t[a];
		for(int ii=0; ii<a; ++ii)
			data[ii] = b;
	}

	NonPOD():NonPOD(4,2) { }

	~NonPOD()
	{
		std::cout << "NonPOD dtor" << std::endl;
		delete[] data;
	}

	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t* data;
};

int main(int argc, char** argv)
{
	HeapArea area(1000);
	MemoryArena<LinearAllocator, policy::SingleThread, policy::SimpleBoundsChecking> arena(area);

	std::cout << "POD is POD: " << std::is_pod<POD>::value << std::endl;
	std::cout << "NonPOD is POD: " << std::is_pod<NonPOD>::value << std::endl;
	std::cout << std::endl;
	
	{
		std::cout << "--- new POD non-aligned ---" << std::endl;
		POD* some_pod = W_NEW(POD, arena);
		std::cout << some_pod << " " << size_t(some_pod)%16 << std::endl;
		some_pod->a = 42;
		some_pod->b = 56;
		some_pod->plop = 5587474657354873254;
		std::cout << some_pod->a << " " << some_pod->b << " " << some_pod->plop << std::endl;
		W_DELETE(some_pod, arena);
		std::cout << std::endl;
	}

	{
		std::cout << "--- new POD aligned ---" << std::endl;
		POD* some_pod = W_NEW_ALIGN(POD, arena, 16);
		std::cout << some_pod << " " << size_t(some_pod)%16 << std::endl;
		some_pod->a = 42;
		some_pod->b = 56;
		some_pod->plop = 5587474657354873254;
		std::cout << some_pod->a << " " << some_pod->b << " " << some_pod->plop << std::endl;
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
		std::cout << pod_array[5].a << " " << pod_array[5].b << std::endl;
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
		std::cout << pod_array[5].a << " " << pod_array[5].b << std::endl;
		W_DELETE_ARRAY(pod_array, arena);
		std::cout << std::endl;
	}

	{
		std::cout << "--- new non-POD non-aligned ---" << std::endl;
		NonPOD* some_non_pod = W_NEW(NonPOD, arena)(10,8);
		std::cout << some_non_pod->data[9] << std::endl;
		W_DELETE(some_non_pod, arena);
		std::cout << std::endl;
	}

	{
		std::cout << "--- new non-POD array non-aligned ---" << std::endl;
		NonPOD* non_pod_array = W_NEW_ARRAY(NonPOD[4], arena);
		W_DELETE_ARRAY(non_pod_array, arena);
		std::cout << std::endl;
	}
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