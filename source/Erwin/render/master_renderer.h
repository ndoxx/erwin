#pragma once

#include <map>

#include "render/render_queue.hpp"
#include "render/buffer.h"
#include "render/shader.h"
#include "render/texture.h"
#include "ctti/type_id.hpp"

namespace erwin
{

class MasterRenderer;
static std::unique_ptr<MasterRenderer> MASTER_RENDERER = nullptr;

struct InstancedSpriteQueueData
{
	uint32_t instance_count = 0;
	WRef<Texture2D> texture = nullptr;
	WRef<VertexArray> VAO = nullptr;
	WRef<UniformBuffer> UBO = nullptr;
	WRef<ShaderStorageBuffer> SSBO = nullptr;

	inline void reset()
	{
		instance_count = 0;
		texture = nullptr;
		VAO = nullptr;
		UBO = nullptr;
		SSBO = nullptr;
	}
};

template <>
struct SortKeyCreator<InstancedSpriteQueueData>
{
	inline uint64_t operator()(const InstancedSpriteQueueData& data)
	{
		return uint8_t(data.UBO->get_unique_id()) // TMP: this is BAD, unique ID is a uint64_t...
		    + (uint8_t(data.texture->get_unique_id()) << 8);
	}
};

class MasterRenderer
{
public:
	static void create();
	static void kill();

	MasterRenderer();
	~MasterRenderer();

	template <typename QueueDataT>
	void add_queue(uint32_t priority, uint32_t num_data, uint32_t num_rs,
				   std::function<void(const QueueDataT&)> on_data,
                   std::function<void(const PassState&)> on_state)
	{
		auto pqueue = std::make_unique<RenderQueue<QueueDataT>>();
		pqueue->resize_pool(num_data, num_rs);
		pqueue->set_command_handlers(on_data, on_state);
		queues_.insert(std::make_pair(ctti::type_id<QueueDataT>(),
									  std::move(pqueue)));
		queue_priority_.insert(std::make_pair(priority, ctti::type_id<QueueDataT>()));
	}

	template <typename QueueDataT>
	inline RenderQueue<QueueDataT>& get_queue()
	{
		return static_cast<RenderQueue<QueueDataT>&>(*queues_.at(ctti::type_id<QueueDataT>()));
	}

	void flush();

	static ShaderBank shader_bank;

private:
	void apply_state(const PassState& state);
	void execute(const InstancedSpriteQueueData& data);

private:
	std::unordered_map<ctti::unnamed_type_id_t, std::unique_ptr<AbstractRenderQueue>> queues_;
	std::multimap<uint32_t, ctti::unnamed_type_id_t> queue_priority_;
	PassState prev_state_;
};


} // namespace erwin