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

void (* backend_dispatch [])(memory::LinearBuffer<>&) =
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
command_buffer_(mem_range),
auxiliary_arena_(aux_mem_range),
head_(0)
{

}

CommandQueue::~CommandQueue()
{

}

void CommandQueue::reset()
{
	//arena_.get_allocator().reset();
	command_buffer_.reset();
	auxiliary_arena_.get_allocator().reset();
	commands_.clear();
	head_ = 0;
}

IndexBufferHandle CommandQueue::create_index_buffer(uint64_t key, uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode)
{
	void* cmd = command_buffer_.get_head();
	IndexBufferHandle handle = { s_index_buffer_handles.acquire() };

	// Write data
	RenderCommand type = RenderCommand::CreateIndexBuffer;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);
	command_buffer_.write(&count);
	command_buffer_.write(&primitive);
	command_buffer_.write(&mode);

	// Write auxiliary data
	uint32_t* auxiliary = nullptr;
	if(index_data)
	{
		auxiliary = W_NEW_ARRAY_DYNAMIC(uint32_t, count, auxiliary_arena_);
		memcpy(auxiliary, index_data, count * sizeof(uint32_t));
	}
	else
	{
		W_ASSERT(mode != DrawMode::Static, "Index data can't be null in static mode.");
	}
	command_buffer_.write(&auxiliary);

	push(cmd, key);
	return handle;
}

VertexBufferLayoutHandle CommandQueue::create_vertex_buffer_layout(uint64_t key, const std::initializer_list<BufferLayoutElement>& elements)
{
	void* cmd = command_buffer_.get_head();
	VertexBufferLayoutHandle handle = { s_vertex_buffer_layout_handles.acquire() };

	std::vector<BufferLayoutElement> elts(elements);
	uint32_t count = elts.size();

	// Write data
	RenderCommand type = RenderCommand::CreateVertexBufferLayout;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);
	command_buffer_.write(&count);

	// Write auxiliary data
	BufferLayoutElement* auxiliary = W_NEW_ARRAY_DYNAMIC(BufferLayoutElement, elts.size(), auxiliary_arena_);
	memcpy(auxiliary, elts.data(), elts.size() * sizeof(BufferLayoutElement));
	command_buffer_.write(&auxiliary);

	push(cmd, key);
	return handle;
}

VertexBufferHandle CommandQueue::create_vertex_buffer(uint64_t key, VertexBufferLayoutHandle layout, float* vertex_data, uint32_t count, DrawMode mode)
{
	W_ASSERT(is_valid(layout), "Invalid VertexBufferLayoutHandle!");

	void* cmd = command_buffer_.get_head();
	VertexBufferHandle handle = { s_vertex_buffer_handles.acquire() };

	// Write data
	RenderCommand type = RenderCommand::CreateVertexBuffer;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);
	command_buffer_.write(&layout);
	command_buffer_.write(&count);
	command_buffer_.write(&mode);

	// Write auxiliary data
	float* auxiliary = nullptr;
	if(vertex_data)
	{
		auxiliary = W_NEW_ARRAY_DYNAMIC(float, count, auxiliary_arena_);
		memcpy(auxiliary, vertex_data, count * sizeof(float));
	}
	else
	{
		W_ASSERT(mode != DrawMode::Static, "Vertex data can't be null in static mode.");
	}
	command_buffer_.write(&auxiliary);

	push(cmd, key);
	return handle;
}

VertexArrayHandle CommandQueue::create_vertex_array(uint64_t key, VertexBufferHandle vb, IndexBufferHandle ib)
{
	W_ASSERT(is_valid(vb), "Invalid VertexBufferHandle!");
	W_ASSERT(is_valid(ib), "Invalid IndexBufferHandle!");

	void* cmd = command_buffer_.get_head();
	VertexArrayHandle handle = { s_vertex_array_handles.acquire() };

	// Write data
	RenderCommand type = RenderCommand::CreateVertexArray;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);
	command_buffer_.write(&vb);
	command_buffer_.write(&ib);

	push(cmd, key);
	return handle;
}

UniformBufferHandle CommandQueue::create_uniform_buffer(uint64_t key, const std::string& name, void* data, uint32_t size, DrawMode mode)
{
	W_ASSERT(name.size()<=20, "UBO layout name should be less than 20 characters long.");
	
	void* cmd = command_buffer_.get_head();
	UniformBufferHandle handle = { s_uniform_buffer_handles.acquire() };

	// Write data
	RenderCommand type = RenderCommand::CreateUniformBuffer;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);
	command_buffer_.write(&size);
	command_buffer_.write(&mode);
	command_buffer_.write_str(name);

	// Write auxiliary data
	uint8_t* auxiliary = nullptr;
	if(data)
	{
		auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, auxiliary_arena_);
		memcpy(auxiliary, data, size);
	}
	else
	{
		W_ASSERT(mode != DrawMode::Static, "UBO data can't be null in static mode.");
	}
	command_buffer_.write(&auxiliary);

	push(cmd, key);
	return handle;
}

ShaderStorageBufferHandle CommandQueue::create_shader_storage_buffer(uint64_t key, const std::string& name, void* data, uint32_t size, DrawMode mode)
{
	W_ASSERT(name.size()<=20, "SSBO layout name should be less than 20 characters long.");
	
	void* cmd = command_buffer_.get_head();
	ShaderStorageBufferHandle handle = { s_shader_storage_buffer_handles.acquire() };

	// Write data
	RenderCommand type = RenderCommand::CreateShaderStorageBuffer;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);
	command_buffer_.write(&size);
	command_buffer_.write(&mode);
	command_buffer_.write_str(name);

	// Write auxiliary data
	uint8_t* auxiliary = nullptr;
	if(data)
	{
		auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, auxiliary_arena_);
		memcpy(auxiliary, data, size);
	}
	else
	{
		W_ASSERT(mode != DrawMode::Static, "SSBO data can't be null in static mode.");
	}
	command_buffer_.write(&auxiliary);

	push(cmd, key);
	return handle;
}

void CommandQueue::update_index_buffer(uint64_t key, IndexBufferHandle handle, uint32_t* data, uint32_t count)
{
	W_ASSERT(is_valid(handle), "Invalid IndexBufferHandle!");
	W_ASSERT(data, "No data!");
	void* cmd = command_buffer_.get_head();
	
	RenderCommand type = RenderCommand::UpdateIndexBuffer;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);
	command_buffer_.write(&count);
	uint32_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint32_t, count, auxiliary_arena_);
	memcpy(auxiliary, data, count);
	command_buffer_.write(&auxiliary);

	push(cmd, key);
}

void CommandQueue::update_vertex_buffer(uint64_t key, VertexBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(is_valid(handle), "Invalid VertexBufferHandle!");
	W_ASSERT(data, "No data!");
	void* cmd = command_buffer_.get_head();
	
	RenderCommand type = RenderCommand::UpdateVertexBuffer;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);
	command_buffer_.write(&size);
	uint8_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, auxiliary_arena_);
	memcpy(auxiliary, data, size);
	command_buffer_.write(&auxiliary);

	push(cmd, key);
}

void CommandQueue::update_uniform_buffer(uint64_t key, UniformBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(is_valid(handle), "Invalid UniformBufferHandle!");
	W_ASSERT(data, "No data!");
	void* cmd = command_buffer_.get_head();
	
	RenderCommand type = RenderCommand::UpdateUniformBuffer;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);
	command_buffer_.write(&size);
	uint8_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, auxiliary_arena_);
	memcpy(auxiliary, data, size);
	command_buffer_.write(&auxiliary);

	push(cmd, key);
}

void CommandQueue::update_shader_storage_buffer(uint64_t key, ShaderStorageBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(is_valid(handle), "Invalid ShaderStorageBufferHandle!");
	W_ASSERT(data, "No data!");
	void* cmd = command_buffer_.get_head();
	
	RenderCommand type = RenderCommand::UpdateShaderStorageBuffer;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);
	command_buffer_.write(&size);
	uint8_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, auxiliary_arena_);
	memcpy(auxiliary, data, size);
	command_buffer_.write(&auxiliary);

	push(cmd, key);
}

void CommandQueue::destroy_index_buffer(uint64_t key, IndexBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid IndexBufferHandle!");
	s_index_buffer_handles.release(handle.index);
	void* cmd = command_buffer_.get_head();
	
	RenderCommand type = RenderCommand::DestroyIndexBuffer;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);

	push(cmd, key);
}

void CommandQueue::destroy_vertex_buffer_layout(uint64_t key, VertexBufferLayoutHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid VertexBufferLayoutHandle!");
	s_vertex_buffer_layout_handles.release(handle.index);
	void* cmd = command_buffer_.get_head();
	
	RenderCommand type = RenderCommand::DestroyVertexBufferLayout;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);

	push(cmd, key);
}

void CommandQueue::destroy_vertex_buffer(uint64_t key, VertexBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid VertexBufferHandle!");
	s_vertex_buffer_handles.release(handle.index);
	void* cmd = command_buffer_.get_head();
	
	RenderCommand type = RenderCommand::DestroyVertexBuffer;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);

	push(cmd, key);
}

void CommandQueue::destroy_vertex_array(uint64_t key, VertexArrayHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid VertexArrayHandle!");
	s_vertex_array_handles.release(handle.index);
	void* cmd = command_buffer_.get_head();
	
	RenderCommand type = RenderCommand::DestroyVertexArray;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);

	push(cmd, key);
}

void CommandQueue::destroy_uniform_buffer(uint64_t key, UniformBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid UniformBufferHandle!");
	s_uniform_buffer_handles.release(handle.index);
	void* cmd = command_buffer_.get_head();
	
	RenderCommand type = RenderCommand::DestroyUniformBuffer;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);

	push(cmd, key);
}

void CommandQueue::destroy_shader_storage_buffer(uint64_t key, ShaderStorageBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid ShaderStorageBufferHandle!");
	s_shader_storage_buffer_handles.release(handle.index);
	void* cmd = command_buffer_.get_head();
	
	RenderCommand type = RenderCommand::DestroyShaderStorageBuffer;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);

	push(cmd, key);
}


} // namespace WIP
} // namespace erwin