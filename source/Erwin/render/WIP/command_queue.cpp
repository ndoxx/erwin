#include "render/WIP/command_queue.h"
#include "render/WIP/master_renderer.h"
#include "memory/handle_pool.h"

namespace erwin
{
namespace WIP
{

namespace detail
{
	inline void nop(RenderCommand*) { }
}

static HandlePoolT<k_max_index_buffers>          s_index_buffer_handles;
static HandlePoolT<k_max_vertex_buffer_layouts>  s_vertex_buffer_layout_handles;
static HandlePoolT<k_max_vertex_buffers>         s_vertex_buffer_handles;
static HandlePoolT<k_max_vertex_arrays>          s_vertex_array_handles;
static HandlePoolT<k_max_uniform_buffers>        s_uniform_buffer_handles;
static HandlePoolT<k_max_shader_storage_buffers> s_shader_storage_buffer_handles;
static HandlePoolT<k_max_textures>               s_texture_handles;
static HandlePoolT<k_max_shaders>                s_shader_handles;

CommandQueue::CommandQueue(std::pair<void*,void*> mem_range, std::pair<void*,void*> aux_mem_range):
arena_(mem_range),
auxiliary_arena_(aux_mem_range),
head_(0)
{

}

CommandQueue::~CommandQueue()
{

}

void CommandQueue::reset()
{
	arena_.get_allocator().reset();
	auxiliary_arena_.get_allocator().reset();
	commands_.clear();
	head_ = 0;
}

IndexBufferHandle CommandQueue::create_index_buffer(uint64_t key, uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode)
{
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::CreateIndexBuffer;
	IndexBufferHandle handle = { s_index_buffer_handles.acquire() };

	// Write data
	cmd->write(&handle);
	cmd->write(&index_data);
	cmd->write(&count);
	cmd->write(&primitive);
	cmd->write(&mode);

	// Set dispatch functions
	cmd->backend_dispatch_func = MasterRenderer::dispatch::create_index_buffer;
	cmd->state_handler_func = &detail::nop;

	push(cmd, key);
	return handle;
}

VertexBufferLayoutHandle CommandQueue::create_vertex_buffer_layout(uint64_t key, const std::initializer_list<BufferLayoutElement>& elements)
{
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::CreateVertexBufferLayout;
	VertexBufferLayoutHandle handle = { s_vertex_buffer_layout_handles.acquire() };

	std::vector<BufferLayoutElement> elts(elements);
	uint32_t count = elts.size();

	// Write data
	cmd->write(&handle);
	cmd->write(&count);

	// Write auxiliary data
	cmd->auxiliary = W_NEW_ARRAY_DYNAMIC(BufferLayoutElement, elts.size(), auxiliary_arena_);
	memcpy(cmd->auxiliary, elts.data(), elts.size() * sizeof(BufferLayoutElement));

	// Set dispatch functions
	cmd->backend_dispatch_func = MasterRenderer::dispatch::create_vertex_buffer_layout;
	cmd->state_handler_func = &detail::nop;

	push(cmd, key);
	return handle;
}

VertexBufferHandle CommandQueue::create_vertex_buffer(uint64_t key, VertexBufferLayoutHandle layout, float* vertex_data, uint32_t count, DrawMode mode)
{
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::CreateVertexBuffer;
	VertexBufferHandle handle = { s_vertex_buffer_handles.acquire() };

	// Write data
	cmd->write(&handle);
	cmd->write(&layout);
	cmd->write(&count);
	cmd->write(&mode);

	// Write auxiliary data
	if(vertex_data)
	{
		cmd->auxiliary = W_NEW_ARRAY_DYNAMIC(float, count, auxiliary_arena_);
		memcpy(cmd->auxiliary, vertex_data, count * sizeof(float));
	}
	else
	{
		W_ASSERT(mode != DrawMode::Static, "Vertex data can't be null in static mode.");
	}

	// Set dispatch functions
	cmd->backend_dispatch_func = MasterRenderer::dispatch::create_vertex_buffer;
	cmd->state_handler_func = &detail::nop;

	push(cmd, key);
	return handle;
}

VertexArrayHandle CommandQueue::create_vertex_array(uint64_t key, VertexBufferHandle vb, IndexBufferHandle ib)
{
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::CreateVertexArray;
	VertexArrayHandle handle = { s_vertex_array_handles.acquire() };

	// Write data
	cmd->write(&handle);
	cmd->write(&vb);
	cmd->write(&ib);

	// Set dispatch functions
	cmd->backend_dispatch_func = MasterRenderer::dispatch::create_vertex_array;
	cmd->state_handler_func = &detail::nop;

	push(cmd, key);
	return handle;
}

UniformBufferHandle CommandQueue::create_uniform_buffer(uint64_t key, const std::string& name, void* data, uint32_t struct_size, DrawMode mode)
{
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::CreateUniformBuffer;
	UniformBufferHandle handle = { s_uniform_buffer_handles.acquire() };

	// Write data
	W_ASSERT(name.size()<=20, "UBO layout name should be less than 20 characters long.")
	cmd->write(&handle);
	cmd->write(&struct_size);
	cmd->write(&mode);
	cmd->write_str(name);

	// Write auxiliary data
	if(data)
	{
		cmd->auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, struct_size, auxiliary_arena_);
		memcpy(cmd->auxiliary, data, struct_size);
	}
	else
	{
		W_ASSERT(mode != DrawMode::Static, "UBO data can't be null in static mode.");
	}

	// Set dispatch functions
	cmd->backend_dispatch_func = MasterRenderer::dispatch::create_uniform_buffer;
	cmd->state_handler_func = &detail::nop;

	push(cmd, key);
	return handle;
}

ShaderStorageBufferHandle CommandQueue::create_shader_storage_buffer(uint64_t key, const std::string& name, void* data, uint32_t size, DrawMode mode)
{
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::CreateShaderStorageBuffer;
	ShaderStorageBufferHandle handle = { s_shader_storage_buffer_handles.acquire() };

	// Write data
	W_ASSERT(name.size()<=20, "SSBO layout name should be less than 20 characters long.")
	cmd->write(&handle);
	cmd->write(&size);
	cmd->write(&mode);
	cmd->write_str(name);

	// Write auxiliary data
	if(data)
	{
		cmd->auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, auxiliary_arena_);
		memcpy(cmd->auxiliary, data, size);
	}
	else
	{
		W_ASSERT(mode != DrawMode::Static, "SSBO data can't be null in static mode.");
	}

	// Set dispatch functions
	cmd->backend_dispatch_func = MasterRenderer::dispatch::create_shader_storage_buffer;
	cmd->state_handler_func = &detail::nop;

	push(cmd, key);
	return handle;
}

void CommandQueue::update_index_buffer(uint64_t key, IndexBufferHandle handle, uint32_t* data, uint32_t count)
{
	W_ASSERT(data, "No data!");
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::UpdateIndexBuffer;

	cmd->write(&handle);
	cmd->write(&count);
	cmd->auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, count*sizeof(uint32_t), auxiliary_arena_);
	memcpy(cmd->auxiliary, data, count*sizeof(uint32_t));

	// Set dispatch functions
	cmd->backend_dispatch_func = MasterRenderer::dispatch::update_index_buffer;
	cmd->state_handler_func = &detail::nop;

	push(cmd, key);
}

void CommandQueue::update_vertex_buffer(uint64_t key, VertexBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(data, "No data!");
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::UpdateVertexBuffer;

	cmd->write(&handle);
	cmd->write(&size);
	cmd->auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, auxiliary_arena_);
	memcpy(cmd->auxiliary, data, size);

	// Set dispatch functions
	cmd->backend_dispatch_func = MasterRenderer::dispatch::update_vertex_buffer;
	cmd->state_handler_func = &detail::nop;

	push(cmd, key);
}

void CommandQueue::update_uniform_buffer(uint64_t key, UniformBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(data, "No data!");
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::UpdateUniformBuffer;

	cmd->write(&handle);
	cmd->write(&size);
	cmd->auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, auxiliary_arena_);
	memcpy(cmd->auxiliary, data, size);

	// Set dispatch functions
	cmd->backend_dispatch_func = MasterRenderer::dispatch::update_uniform_buffer;
	cmd->state_handler_func = &detail::nop;

	push(cmd, key);
}

void CommandQueue::update_shader_storage_buffer(uint64_t key, ShaderStorageBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(data, "No data!");
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::UpdateShaderStorageBuffer;

	cmd->write(&handle);
	cmd->write(&size);
	cmd->auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, auxiliary_arena_);
	memcpy(cmd->auxiliary, data, size);

	// Set dispatch functions
	cmd->backend_dispatch_func = MasterRenderer::dispatch::update_shader_storage_buffer;
	cmd->state_handler_func = &detail::nop;

	push(cmd, key);
}

void CommandQueue::destroy_index_buffer(uint64_t key, IndexBufferHandle handle)
{
	s_index_buffer_handles.release(handle.index);
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::DestroyIndexBuffer;
	cmd->write(&handle);

	cmd->backend_dispatch_func = MasterRenderer::dispatch::destroy_index_buffer;
	cmd->state_handler_func = &detail::nop;
	push(cmd, key);
}

void CommandQueue::destroy_vertex_buffer_layout(uint64_t key, VertexBufferLayoutHandle handle)
{
	s_vertex_buffer_layout_handles.release(handle.index);
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::DestroyVertexBufferLayout;
	cmd->write(&handle);

	cmd->backend_dispatch_func = MasterRenderer::dispatch::destroy_vertex_buffer_layout;
	cmd->state_handler_func = &detail::nop;
	push(cmd, key);
}

void CommandQueue::destroy_vertex_buffer(uint64_t key, VertexBufferHandle handle)
{
	s_vertex_buffer_handles.release(handle.index);
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::DestroyVertexBuffer;
	cmd->write(&handle);

	cmd->backend_dispatch_func = MasterRenderer::dispatch::destroy_vertex_buffer;
	cmd->state_handler_func = &detail::nop;
	push(cmd, key);
}

void CommandQueue::destroy_vertex_array(uint64_t key, VertexArrayHandle handle)
{
	s_vertex_array_handles.release(handle.index);
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::DestroyVertexArray;
	cmd->write(&handle);

	cmd->backend_dispatch_func = MasterRenderer::dispatch::destroy_vertex_array;
	cmd->state_handler_func = &detail::nop;
	push(cmd, key);
}

void CommandQueue::destroy_uniform_buffer(uint64_t key, UniformBufferHandle handle)
{
	s_uniform_buffer_handles.release(handle.index);
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::DestroyUniformBuffer;
	cmd->write(&handle);

	cmd->backend_dispatch_func = MasterRenderer::dispatch::destroy_uniform_buffer;
	cmd->state_handler_func = &detail::nop;
	push(cmd, key);
}

void CommandQueue::destroy_shader_storage_buffer(uint64_t key, ShaderStorageBufferHandle handle)
{
	s_shader_storage_buffer_handles.release(handle.index);
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::DestroyShaderStorageBuffer;
	cmd->write(&handle);

	cmd->backend_dispatch_func = MasterRenderer::dispatch::destroy_shader_storage_buffer;
	cmd->state_handler_func = &detail::nop;
	push(cmd, key);
}


} // namespace WIP
} // namespace erwin