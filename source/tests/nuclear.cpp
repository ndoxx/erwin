#include <iostream>
#include <vector>
#include <functional>
#include <random>
#include <type_traits>

#include "render/render_queue.hpp"


using namespace erwin;

struct InstancedSpriteQueueData
{
	typedef uint64_t RenderKey;

	hash_t shader;
	hash_t texture;
	uint32_t SSBO;
	uint32_t UBO;

	inline void reset()
	{
		shader = texture = SSBO = UBO = 0;
	}
};

template <>
void RenderQueue<InstancedSpriteQueueData>::make_key(RenderQueue<InstancedSpriteQueueData>::QueueItemT& item)
{
	if(!item.is_state)
		item.key = uint8_t(item.content.data->texture) + (uint8_t(item.content.data->shader) << 8);

	item.key |= item.is_state ? (1<<16) : 0;
	item.key |= (~current_pass_ << 17);
}

int main(int argc, char** argv)
{
	std::default_random_engine generator;
	std::uniform_int_distribution<uint64_t> dis3(0,2);
	std::uniform_int_distribution<uint32_t> dis10(0,9);

	RenderQueue<InstancedSpriteQueueData> isp_queue;
	isp_queue.resize_pool(8192,32);

	for(int jj=0; jj<3; ++jj)
	{
		auto* state = isp_queue.render_state_ptr();
		state->render_target = jj;
		isp_queue.push(state);

		for(int ii=0; ii<20; ++ii)
		{
			auto* data = isp_queue.data_ptr();
			data->shader  = uint64_t(ii%2);
			data->texture = dis3(generator);
			data->SSBO    = dis10(generator);
			data->UBO     = dis10(generator);
			isp_queue.push(data);
		}
	}

	isp_queue.flush(
	[&](const InstancedSpriteQueueData& data)
	{
		std::cout << "[" << data.shader << "," << data.texture 
				  << "," << data.SSBO << "," << data.UBO << "]" << std::endl;
	},
	[&](const RenderState& state)
	{
		std::cout << "[STATE rt: " << state.render_target << "]" << std::endl;
	});


	return 0;
}