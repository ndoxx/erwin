#pragma once

#include <map>
#include <iostream>

#include "render/render_queue.hpp"
#include "render/buffer.h"
#include "render/shader.h"
#include "render/texture.h"
#include "core/unique_id.h"
#include "ctti/type_id.hpp"

#include <thread>

namespace erwin
{

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

struct PostProcessingQueueData
{
	hash_t input_framebuffer = 0;
	uint32_t framebuffer_texture_index = 0;
	WRef<VertexArray> VAO = nullptr;
	WRef<UniformBuffer> UBO = nullptr;

	inline void reset()
	{
		input_framebuffer = 0;
		framebuffer_texture_index = 0;
		VAO = nullptr;
		UBO = nullptr;
	}
};

template <>
struct SortKeyCreator<PostProcessingQueueData>
{
	inline uint64_t operator()(const PostProcessingQueueData& data)
	{
		return uint8_t(data.framebuffer_texture_index)
		    + (uint8_t(data.input_framebuffer) << 8);
	}
};


class QueryTimer;
class MasterRenderer
{
public:
	static void create();
	static void kill();
	static inline MasterRenderer& instance() { return *instance_; }

	MasterRenderer();
	~MasterRenderer();

	inline void set_profiling_enabled(bool value = true) { profiling_enabled_ = value; }

	template <typename QueueDataT>
	void add_queue(uint32_t priority, uint32_t num_data, uint32_t num_rs,
				   std::function<void(const QueueDataT&)> on_data,
                   std::function<void(const PassState&)> on_state)
	{
		auto pqueue = std::make_unique<RenderQueue<QueueDataT>>();
		pqueue->resize_pool(num_data, num_rs);
		pqueue->set_command_handlers(on_data, on_state);
		queues_.insert(std::make_pair(ctti::type_id<QueueDataT>(), std::move(pqueue)));
		queue_priority_.insert(std::make_pair(priority, ctti::type_id<QueueDataT>()));
	}

	template <typename QueueDataT>
	bool has_queue() const
	{
		return (queues_.find(ctti::type_id<QueueDataT>())!=queues_.end());
	}

	template <typename QueueDataT>
	RenderQueue<QueueDataT>& get_queue()
	{
		return static_cast<RenderQueue<QueueDataT>&>(*queues_.at(ctti::type_id<QueueDataT>()));
	}

	void flush();
	void on_imgui_render();

	static ShaderBank shader_bank;

private:
	void apply_state(const PassState& state);
	void execute_isp(const InstancedSpriteQueueData& data);
	void execute_pp(const PostProcessingQueueData& data);

	inline void reset_stats() { stats_.render_time = 0.f; }

	struct RenderStats
	{
		float render_time = 0.f;
	};
	struct StateCache
	{
		W_ID shader = 0;
		W_ID texture = 0;
		W_ID VAO = 0;
		W_ID UBO = 0;
		W_ID SSBO = 0;
	};

private:
	std::unordered_map<ctti::unnamed_type_id_t, std::unique_ptr<AbstractRenderQueue>> queues_;
	std::multimap<uint32_t, ctti::unnamed_type_id_t> queue_priority_;
	PassState prev_state_;
	StateCache state_cache_;

	bool profiling_enabled_;
	WScope<QueryTimer> query_timer_;
	RenderStats stats_;

	static std::unique_ptr<MasterRenderer> instance_;
};


} // namespace erwin