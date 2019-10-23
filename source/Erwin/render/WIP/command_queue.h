#pragma once

#include <cstdint>
#include <cstring>
#include <algorithm>
#include <vector>
#include <functional>
#include <iostream>

#include "core/core.h"
#include "render/buffer_layout.h"
#include "render/render_state.h"
// #define ARENA_RETAIL
#include "memory/arena.h"

namespace erwin
{
namespace WIP
{

// Handle structures to manipulate graphics objects
constexpr uint32_t k_invalid_handle = 0xffff;

#define W_HANDLE(name)                                                     \
	struct name 														   \
	{ 																	   \
		inline bool is_valid() const { return index != k_invalid_handle; } \
		inline void invalidate()     { index = k_invalid_handle; }         \
		uint32_t index;													   \
	};																	   \

W_HANDLE(IndexBufferHandle);
W_HANDLE(VertexBufferHandle);
W_HANDLE(VertexBufferLayoutHandle);
W_HANDLE(VertexArrayHandle);
W_HANDLE(UniformBufferHandle);
W_HANDLE(ShaderStorageBufferHandle);
W_HANDLE(TextureHandle);
W_HANDLE(ShaderHandle);

constexpr std::size_t k_max_render_command_data_size = 32;

// Pushed to a command queue
struct RenderCommand
{
	enum: uint16_t
	{
		CreateIndexBuffer,
		CreateVertexBufferLayout,
		CreateVertexBuffer,
		CreateVertexArray,
		CreateUniformBuffer,
		CreateShaderStorageBuffer,

		Count
	};

	inline void reset() { render_state = 0; head = 0; auxiliary = nullptr; backend_dispatch_func = nullptr; }
	inline void write(void const* source, std::size_t size)
	{
		W_ASSERT(size + head < k_max_render_command_data_size, "[RenderCommand] Data buffer overwrite!");
		memcpy(data + head, source, size);
		head += (uint16_t)size;
	}
	inline void read(void* destination, std::size_t size)
	{
		W_ASSERT(int(head) - size >= 0, "[RenderCommand] Data buffer empty!");
		memcpy(destination, data + head - size, size);
		head -= (uint16_t)size;
	}

	uint64_t render_state;
	uint16_t type;
	uint16_t head;
	uint8_t  data[k_max_render_command_data_size];

	void* auxiliary;
	void (*backend_dispatch_func)(RenderCommand*);
	void (*state_handler_func)(RenderCommand*);
};

class CommandQueue
{
public:
	typedef std::pair<uint64_t,RenderCommand*> QueueItem;

	enum
	{
		Resource = 0,
		Instanced2D,

		Count
	};

	CommandQueue(std::pair<void*,void*> mem_range, std::pair<void*,void*> aux_mem_range);
	~CommandQueue();

	// The following functions will initialize a render command and push it to this queue 
	void create_index_buffer(uint64_t key, IndexBufferHandle* handle, uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode = DrawMode::Static);
	void create_vertex_buffer_layout(uint64_t key, VertexBufferLayoutHandle* handle, const std::initializer_list<BufferLayoutElement>& elements);
	void create_vertex_buffer(uint64_t key, VertexBufferHandle* handle, VertexBufferLayoutHandle* layout, float* vertex_data, uint32_t count, DrawMode mode = DrawMode::Static);

	// Sort queue by sorting key
	inline void sort()
	{
		// Keys stored separately from commands to avoid touching data too
		// much during sort calls
        std::sort(std::begin(commands_), std::end(commands_), 
	        [&](const QueueItem& item1, const QueueItem& item2)
	        {
	        	return item1.first > item2.first;
	        });
	}

	// Dispatch all commands
	inline void flush()
	{
		for(auto&& [key,cmd]: commands_)
		{
			(*cmd->state_handler_func)(cmd);
			(*cmd->backend_dispatch_func)(cmd);
			W_DELETE(cmd, arena_);
		}
	}

	// Clear queue
	void reset();

private:
	inline RenderCommand* get()
	{
		return W_NEW(RenderCommand, arena_);
	}

	inline void push(RenderCommand* cmd, uint64_t key)
	{
		commands_.push_back({key, cmd});
	}

private:
	typedef memory::MemoryArena<memory::LinearAllocator, 
			    				memory::policy::SingleThread, 
			    				memory::policy::NoBoundsChecking,
			    				memory::policy::NoMemoryTagging,
			    				memory::policy::NoMemoryTracking> AuxArena;

	LinearArena arena_;
	AuxArena auxiliary_arena_;

	std::size_t head_;
	std::vector<QueueItem> commands_; // TODO: Find a faster alternative
};

} // namespace WIP
} // namespace erwin