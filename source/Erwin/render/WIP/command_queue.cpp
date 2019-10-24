#include "render/WIP/command_queue.h"
#include "render/WIP/master_renderer.h"
#include "memory/handle_pool.h"

namespace erwin
{
namespace WIP
{

static HandlePoolT<k_max_index_buffers>          s_index_buffer_handles;
static HandlePoolT<k_max_vertex_buffer_layouts>  s_vertex_buffer_layout_handles;
static HandlePoolT<k_max_vertex_buffers>         s_vertex_buffer_handles;
static HandlePoolT<k_max_vertex_arrays>          s_vertex_array_handles;
static HandlePoolT<k_max_uniform_buffers>        s_uniform_buffer_handles;
static HandlePoolT<k_max_shader_storage_buffers> s_shader_storage_buffer_handles;
static HandlePoolT<k_max_textures>               s_texture_handles;
static HandlePoolT<k_max_shaders>                s_shader_handles;

bool is_valid(IndexBufferHandle handle)         { return s_index_buffer_handles.is_valid(handle.index); }
bool is_valid(VertexBufferLayoutHandle handle)  { return s_vertex_buffer_layout_handles.is_valid(handle.index); }
bool is_valid(VertexBufferHandle handle)        { return s_vertex_buffer_handles.is_valid(handle.index); }
bool is_valid(VertexArrayHandle handle)         { return s_vertex_array_handles.is_valid(handle.index); }
bool is_valid(UniformBufferHandle handle)       { return s_uniform_buffer_handles.is_valid(handle.index); }
bool is_valid(ShaderStorageBufferHandle handle) { return s_shader_storage_buffer_handles.is_valid(handle.index); }
bool is_valid(TextureHandle handle)             { return s_texture_handles.is_valid(handle.index); }
bool is_valid(ShaderHandle handle)              { return s_shader_handles.is_valid(handle.index); }

void (* backend_dispatch [])(RenderCommand*) =
{
	&MasterRenderer::dispatch::create_index_buffer,
	&MasterRenderer::dispatch::create_vertex_buffer_layout,
	&MasterRenderer::dispatch::create_vertex_buffer,
	&MasterRenderer::dispatch::create_vertex_array,
	&MasterRenderer::dispatch::create_uniform_buffer,
	&MasterRenderer::dispatch::create_shader_storage_buffer,
	&MasterRenderer::dispatch::update_index_buffer,
	&MasterRenderer::dispatch::update_vertex_buffer,
	&MasterRenderer::dispatch::update_uniform_buffer,
	&MasterRenderer::dispatch::update_shader_storage_buffer,
	&MasterRenderer::dispatch::destroy_index_buffer,
	&MasterRenderer::dispatch::destroy_vertex_buffer_layout,
	&MasterRenderer::dispatch::destroy_vertex_buffer,
	&MasterRenderer::dispatch::destroy_vertex_array,
	&MasterRenderer::dispatch::destroy_uniform_buffer,
	&MasterRenderer::dispatch::destroy_shader_storage_buffer,
};

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
	cmd->write(&count);
	cmd->write(&primitive);
	cmd->write(&mode);

	// Write auxiliary data
	if(index_data)
	{
		cmd->auxiliary = W_NEW_ARRAY_DYNAMIC(uint32_t, count, auxiliary_arena_);
		memcpy(cmd->auxiliary, index_data, count * sizeof(uint32_t));
	}
	else
	{
		W_ASSERT(mode != DrawMode::Static, "Index data can't be null in static mode.");
	}


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

	push(cmd, key);
	return handle;
}

VertexBufferHandle CommandQueue::create_vertex_buffer(uint64_t key, VertexBufferLayoutHandle layout, float* vertex_data, uint32_t count, DrawMode mode)
{
	W_ASSERT(is_valid(layout), "Invalid VertexBufferLayoutHandle!");

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

	push(cmd, key);
	return handle;
}

VertexArrayHandle CommandQueue::create_vertex_array(uint64_t key, VertexBufferHandle vb, IndexBufferHandle ib)
{
	W_ASSERT(is_valid(vb), "Invalid VertexBufferHandle!");
	W_ASSERT(is_valid(ib), "Invalid IndexBufferHandle!");

	RenderCommand* cmd = get();
	cmd->type = RenderCommand::CreateVertexArray;
	VertexArrayHandle handle = { s_vertex_array_handles.acquire() };

	// Write data
	cmd->write(&handle);
	cmd->write(&vb);
	cmd->write(&ib);

	push(cmd, key);
	return handle;
}

UniformBufferHandle CommandQueue::create_uniform_buffer(uint64_t key, const std::string& name, void* data, uint32_t size, DrawMode mode)
{
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::CreateUniformBuffer;
	UniformBufferHandle handle = { s_uniform_buffer_handles.acquire() };

	// Write data
	W_ASSERT(name.size()<=20, "UBO layout name should be less than 20 characters long.")
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
		W_ASSERT(mode != DrawMode::Static, "UBO data can't be null in static mode.");
	}

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

	push(cmd, key);
	return handle;
}

void CommandQueue::update_index_buffer(uint64_t key, IndexBufferHandle handle, uint32_t* data, uint32_t count)
{
	W_ASSERT(is_valid(handle), "Invalid IndexBufferHandle!");
	W_ASSERT(data, "No data!");
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::UpdateIndexBuffer;

	cmd->write(&handle);
	cmd->write(&count);
	cmd->auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, count*sizeof(uint32_t), auxiliary_arena_);
	memcpy(cmd->auxiliary, data, count*sizeof(uint32_t));

	push(cmd, key);
}

void CommandQueue::update_vertex_buffer(uint64_t key, VertexBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(is_valid(handle), "Invalid VertexBufferHandle!");
	W_ASSERT(data, "No data!");
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::UpdateVertexBuffer;

	cmd->write(&handle);
	cmd->write(&size);
	cmd->auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, auxiliary_arena_);
	memcpy(cmd->auxiliary, data, size);

	push(cmd, key);
}

void CommandQueue::update_uniform_buffer(uint64_t key, UniformBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(is_valid(handle), "Invalid UniformBufferHandle!");
	W_ASSERT(data, "No data!");
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::UpdateUniformBuffer;

	cmd->write(&handle);
	cmd->write(&size);
	cmd->auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, auxiliary_arena_);
	memcpy(cmd->auxiliary, data, size);

	push(cmd, key);
}

void CommandQueue::update_shader_storage_buffer(uint64_t key, ShaderStorageBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(is_valid(handle), "Invalid ShaderStorageBufferHandle!");
	W_ASSERT(data, "No data!");
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::UpdateShaderStorageBuffer;

	cmd->write(&handle);
	cmd->write(&size);
	cmd->auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, auxiliary_arena_);
	memcpy(cmd->auxiliary, data, size);

	push(cmd, key);
}

void CommandQueue::destroy_index_buffer(uint64_t key, IndexBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid IndexBufferHandle!");
	s_index_buffer_handles.release(handle.index);
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::DestroyIndexBuffer;
	cmd->write(&handle);

	push(cmd, key);
}

void CommandQueue::destroy_vertex_buffer_layout(uint64_t key, VertexBufferLayoutHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid VertexBufferLayoutHandle!");
	s_vertex_buffer_layout_handles.release(handle.index);
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::DestroyVertexBufferLayout;
	cmd->write(&handle);

	push(cmd, key);
}

void CommandQueue::destroy_vertex_buffer(uint64_t key, VertexBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid VertexBufferHandle!");
	s_vertex_buffer_handles.release(handle.index);
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::DestroyVertexBuffer;
	cmd->write(&handle);

	push(cmd, key);
}

void CommandQueue::destroy_vertex_array(uint64_t key, VertexArrayHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid VertexArrayHandle!");
	s_vertex_array_handles.release(handle.index);
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::DestroyVertexArray;
	cmd->write(&handle);

	push(cmd, key);
}

void CommandQueue::destroy_uniform_buffer(uint64_t key, UniformBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid UniformBufferHandle!");
	s_uniform_buffer_handles.release(handle.index);
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::DestroyUniformBuffer;
	cmd->write(&handle);

	push(cmd, key);
}

void CommandQueue::destroy_shader_storage_buffer(uint64_t key, ShaderStorageBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid ShaderStorageBufferHandle!");
	s_shader_storage_buffer_handles.release(handle.index);
	RenderCommand* cmd = get();
	cmd->type = RenderCommand::DestroyShaderStorageBuffer;
	cmd->write(&handle);

	push(cmd, key);
}


} // namespace WIP
} // namespace erwin