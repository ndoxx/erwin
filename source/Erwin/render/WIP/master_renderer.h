#pragma once

#include <cstdint>
#include <utility>

#include "render/render_state.h"
#include "render/buffer_layout.h"
#include "memory/arena.h"

namespace erwin
{
namespace WIP
{

class CommandQueue;
class MasterRenderer
{
public:
	friend class CommandQueue;

	static void init();
	static void shutdown();

	static CommandQueue& get_queue(int name);
	static void flush();

	static void DEBUG_test();
};

// Handle structures to manipulate graphics objects
enum HandleType: uint16_t
{
	IndexBufferHandleT,
	VertexBufferLayoutHandleT,
	VertexBufferHandleT,
	VertexArrayHandleT,
	UniformBufferHandleT,
	ShaderStorageBufferHandleT,
	TextureHandleT,
	ShaderHandleT,

	Count
};

#define W_HANDLE(name)   							            \
	struct name 		 							            \
	{ 					 							            \
		static constexpr HandleType type = HandleType::name##T; \
		uint32_t index;	 							  			\
	};					 							  			\
	bool is_valid(name); 							  			\

W_HANDLE(IndexBufferHandle);
W_HANDLE(VertexBufferLayoutHandle);
W_HANDLE(VertexBufferHandle);
W_HANDLE(VertexArrayHandle);
W_HANDLE(UniformBufferHandle);
W_HANDLE(ShaderStorageBufferHandle);
W_HANDLE(TextureHandle);
W_HANDLE(ShaderHandle);

#undef W_HANDLE

constexpr std::size_t k_max_render_commands = 1024;
constexpr std::size_t k_max_handles[HandleType::Count] =
{
	128, // index buffers
	128, // vertex buffer layouts
	128, // vertex buffers
	128, // vertex arrays
	128, // uniform buffers
	128, // shader storage buffers
	128, // textures
	128, // shaders
};

class CommandQueue
{
public:
	typedef std::pair<uint64_t,void*> QueueItem;

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

	enum QueueName
	{
		Resource = 0,
		Instanced2D,

		Count
	};

	CommandQueue(std::pair<void*,void*> mem_range, std::pair<void*,void*> aux_mem_range);
	~CommandQueue();

	// The following functions will initialize a render command and push it to this queue 
	IndexBufferHandle         create_index_buffer(uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode = DrawMode::Static);
	VertexBufferLayoutHandle  create_vertex_buffer_layout(const std::initializer_list<BufferLayoutElement>& elements);
	VertexBufferHandle        create_vertex_buffer(VertexBufferLayoutHandle layout, float* vertex_data, uint32_t count, DrawMode mode = DrawMode::Static);
	VertexArrayHandle         create_vertex_array(VertexBufferHandle vb, IndexBufferHandle ib);
	UniformBufferHandle       create_uniform_buffer(const std::string& name, void* data, uint32_t size, DrawMode mode = DrawMode::Dynamic);
	ShaderStorageBufferHandle create_shader_storage_buffer(const std::string& name, void* data, uint32_t size, DrawMode mode = DrawMode::Dynamic);

	void update_index_buffer(IndexBufferHandle handle, uint32_t* data, uint32_t count);
	void update_vertex_buffer(VertexBufferHandle handle, void* data, uint32_t size);
	void update_uniform_buffer(UniformBufferHandle handle, void* data, uint32_t size);
	void update_shader_storage_buffer(ShaderStorageBufferHandle handle, void* data, uint32_t size);

	void destroy_index_buffer(IndexBufferHandle handle);
	void destroy_vertex_buffer_layout(VertexBufferLayoutHandle handle);
	void destroy_vertex_buffer(VertexBufferHandle handle);
	void destroy_vertex_array(VertexArrayHandle handle);
	void destroy_uniform_buffer(UniformBufferHandle handle);
	void destroy_shader_storage_buffer(ShaderStorageBufferHandle handle);

	// Sort queue by sorting key
	inline void sort()
	{
		// Keys stored separately from commands to avoid touching data too
		// much during sort calls
        std::sort(std::begin(commands_), std::begin(commands_) + count_, 
	        [&](const QueueItem& item1, const QueueItem& item2)
	        {
	        	return item1.first > item2.first;
	        });
	}

	// Dispatch all commands
	void flush();
	// Clear queue
	void reset();

	// DEBUG
	inline const void* get_command_buffer() const   { return command_buffer_.begin(); }
	inline const void* get_auxiliary_buffer() const { return auxiliary_arena_.get_allocator().begin(); }

private:

	inline void push(void* cmd, uint64_t key)
	{
		commands_[count_++] = {key, cmd};
	}

private:
	typedef memory::MemoryArena<memory::LinearAllocator, 
			    				memory::policy::SingleThread, 
			    				memory::policy::NoBoundsChecking,
			    				memory::policy::NoMemoryTagging,
			    				memory::policy::NoMemoryTracking> AuxArena;

	memory::LinearBuffer<> command_buffer_;
	AuxArena auxiliary_arena_;

	std::size_t count_;
	QueueItem commands_[k_max_render_commands];
};

} // namespace WIP
} // namespace erwin