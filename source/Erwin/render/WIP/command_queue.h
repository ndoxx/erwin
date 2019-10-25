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
#define W_HANDLE(name)   \
	struct name 		 \
	{ 					 \
		uint32_t index;	 \
	};					 \
	bool is_valid(name); \

W_HANDLE(IndexBufferHandle);
W_HANDLE(VertexBufferHandle);
W_HANDLE(VertexBufferLayoutHandle);
W_HANDLE(VertexArrayHandle);
W_HANDLE(UniformBufferHandle);
W_HANDLE(ShaderStorageBufferHandle);
W_HANDLE(TextureHandle);
W_HANDLE(ShaderHandle);

constexpr std::size_t k_max_render_command_data_size = 32;
constexpr std::size_t k_max_index_buffers            = 128;
constexpr std::size_t k_max_vertex_buffer_layouts    = 128;
constexpr std::size_t k_max_vertex_buffers           = 128;
constexpr std::size_t k_max_vertex_arrays            = 128;
constexpr std::size_t k_max_uniform_buffers          = 128;
constexpr std::size_t k_max_shader_storage_buffers   = 128;
constexpr std::size_t k_max_textures                 = 128;
constexpr std::size_t k_max_shaders                  = 128;

// Pushed to a command queue
enum class RenderCommand: uint16_t
{
	CreateIndexBuffer,
	CreateVertexBufferLayout,
	CreateVertexBuffer,
	CreateVertexArray,
	CreateUniformBuffer,
	CreateShaderStorageBuffer,

	UpdateIndexBuffer,
	UpdateVertexBuffer,
	UpdateUniformBuffer,
	UpdateShaderStorageBuffer,

	DestroyIndexBuffer,
	DestroyVertexBufferLayout,
	DestroyVertexBuffer,
	DestroyVertexArray,
	DestroyUniformBuffer,
	DestroyShaderStorageBuffer,

	Count
};

// Array of function pointers for render command dispatching
extern void (* backend_dispatch [])(memory::LinearBuffer<>&);

class CommandQueue
{
public:
	typedef std::pair<uint64_t,void*> QueueItem;

	enum
	{
		Resource = 0,
		Instanced2D,

		Count
	};

	CommandQueue(std::pair<void*,void*> mem_range, std::pair<void*,void*> aux_mem_range);
	~CommandQueue();

	// The following functions will initialize a render command and push it to this queue 
	IndexBufferHandle         create_index_buffer(uint64_t key, uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode = DrawMode::Static);
	VertexBufferLayoutHandle  create_vertex_buffer_layout(uint64_t key, const std::initializer_list<BufferLayoutElement>& elements);
	VertexBufferHandle        create_vertex_buffer(uint64_t key, VertexBufferLayoutHandle layout, float* vertex_data, uint32_t count, DrawMode mode = DrawMode::Static);
	VertexArrayHandle         create_vertex_array(uint64_t key, VertexBufferHandle vb, IndexBufferHandle ib);
	UniformBufferHandle       create_uniform_buffer(uint64_t key, const std::string& name, void* data, uint32_t size, DrawMode mode = DrawMode::Dynamic);
	ShaderStorageBufferHandle create_shader_storage_buffer(uint64_t key, const std::string& name, void* data, uint32_t size, DrawMode mode = DrawMode::Dynamic);

	void update_index_buffer(uint64_t key, IndexBufferHandle handle, uint32_t* data, uint32_t count);
	void update_vertex_buffer(uint64_t key, VertexBufferHandle handle, void* data, uint32_t size);
	void update_uniform_buffer(uint64_t key, UniformBufferHandle handle, void* data, uint32_t size);
	void update_shader_storage_buffer(uint64_t key, ShaderStorageBufferHandle handle, void* data, uint32_t size);

	void destroy_index_buffer(uint64_t key, IndexBufferHandle handle);
	void destroy_vertex_buffer_layout(uint64_t key, VertexBufferLayoutHandle handle);
	void destroy_vertex_buffer(uint64_t key, VertexBufferHandle handle);
	void destroy_vertex_array(uint64_t key, VertexArrayHandle handle);
	void destroy_uniform_buffer(uint64_t key, UniformBufferHandle handle);
	void destroy_shader_storage_buffer(uint64_t key, ShaderStorageBufferHandle handle);

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
			// TODO: handle state
			command_buffer_.seek(cmd);
			uint16_t type;
			command_buffer_.read(&type);
			(*backend_dispatch[type])(command_buffer_);
		}
	}

	// Clear queue
	void reset();

private:

	inline void push(void* cmd, uint64_t key)
	{
		commands_.push_back({key, cmd});
		++head_;
	}

private:
	typedef memory::MemoryArena<memory::LinearAllocator, 
			    				memory::policy::SingleThread, 
			    				memory::policy::NoBoundsChecking,
			    				memory::policy::NoMemoryTagging,
			    				memory::policy::NoMemoryTracking> AuxArena;

	memory::LinearBuffer<> command_buffer_;
	AuxArena auxiliary_arena_;

	std::size_t head_;
	std::vector<QueueItem> commands_; // TODO: Find a faster alternative
};

} // namespace WIP
} // namespace erwin