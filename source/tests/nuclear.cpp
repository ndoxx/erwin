#include <iostream>
#include <vector>
#include <functional>
#include <random>
#include <type_traits>

#include "render/render_queue.hpp"


using namespace erwin;

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