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

// #include "render/render_queue.hpp"
#include "render/WIP/command_queue.h"

#include "memory/handle_pool.h"
#include "memory/memory_utils.h"

using namespace erwin;
using namespace WIP;



int main(int argc, char** argv)
{
	HandlePoolT<16> handle_pool;
	uint32_t pool_size = sizeof(HandlePoolT<16>);
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(&handle_pool), pool_size);

	uint16_t h1 = handle_pool.acquire();
	uint16_t h2 = handle_pool.acquire();
	uint16_t h3 = handle_pool.acquire();
	uint16_t h4 = handle_pool.acquire();
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(&handle_pool), pool_size);

	std::cout << h1 << " " << h2 << " " << h3 << " " << h4 << std::endl;
	std::cout << handle_pool.is_valid(h1) << " "
			  << handle_pool.is_valid(h2) << " "
			  << handle_pool.is_valid(h3) << " "
			  << handle_pool.is_valid(h4) << " "
			  << handle_pool.is_valid(12) << std::endl;

	handle_pool.release(h2);
	handle_pool.release(h4);
	std::cout << handle_pool.is_valid(h1) << " "
			  << handle_pool.is_valid(h2) << " "
			  << handle_pool.is_valid(h3) << " "
			  << handle_pool.is_valid(h4) << std::endl;
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(&handle_pool), pool_size);

	uint16_t h5 = handle_pool.acquire();
	uint16_t h6 = handle_pool.acquire();
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(&handle_pool), pool_size);
	std::cout << h1 << " " << h3 << " " << h5 << " " << h6 << std::endl;

	return 0;
}

/*
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