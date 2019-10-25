#include "render/WIP/master_renderer.h"

#include <memory>
#include <iostream>

#include "render/buffer.h"
#include "debug/logger.h"
#include "memory/arena.h"
#include "memory/memory_utils.h"
#include "memory/handle_pool.h"
#include "memory/linear_allocator.h"
#include "memory/handle_pool.h"

namespace erwin
{
namespace WIP
{

/*
		   _____ _                             
		  / ____| |                            
		 | (___ | |_ ___  _ __ __ _  __ _  ___ 
		  \___ \| __/ _ \| '__/ _` |/ _` |/ _ \
		  ____) | || (_) | | | (_| | (_| |  __/
		 |_____/ \__\___/|_|  \__,_|\__, |\___|
		                             __/ |     
		                            |___/      
*/

constexpr std::size_t k_handle_alloc_size = 2 * sizeof(uint16_t) * (k_max_handles[HandleType::IndexBufferHandleT]
 										  						  + k_max_handles[HandleType::VertexBufferLayoutHandleT]
 										  						  + k_max_handles[HandleType::VertexBufferHandleT]
 										  						  + k_max_handles[HandleType::VertexArrayHandleT]
 										  						  + k_max_handles[HandleType::UniformBufferHandleT]
 										  						  + k_max_handles[HandleType::ShaderStorageBufferHandleT]
 										  						  + k_max_handles[HandleType::TextureHandleT]
 										  						  + k_max_handles[HandleType::ShaderHandleT]) + 512_B;

struct RendererStorage
{
	RendererStorage():
	renderer_memory(10_MB),
	handle_arena_(renderer_memory.require_block(k_handle_alloc_size))
	{
		std::fill(std::begin(index_buffers), std::end(index_buffers), nullptr);
		std::fill(std::begin(vertex_buffer_layouts), std::end(vertex_buffer_layouts), nullptr);
		std::fill(std::begin(vertex_buffers), std::end(vertex_buffers), nullptr);
		std::fill(std::begin(vertex_arrays), std::end(vertex_arrays), nullptr);
		std::fill(std::begin(uniform_buffers), std::end(uniform_buffers), nullptr);
		std::fill(std::begin(shader_storage_buffers), std::end(shader_storage_buffers), nullptr);

#define MAKE_HANDLE_POOL( TYPE ) handles_[ TYPE ] = W_NEW(HandlePoolT<k_max_handles[ TYPE ]>, handle_arena_)
		MAKE_HANDLE_POOL(HandleType::IndexBufferHandleT);
		MAKE_HANDLE_POOL(HandleType::VertexBufferLayoutHandleT);
		MAKE_HANDLE_POOL(HandleType::VertexBufferHandleT);
		MAKE_HANDLE_POOL(HandleType::VertexArrayHandleT);
		MAKE_HANDLE_POOL(HandleType::UniformBufferHandleT);
		MAKE_HANDLE_POOL(HandleType::ShaderStorageBufferHandleT);
		MAKE_HANDLE_POOL(HandleType::TextureHandleT);
		MAKE_HANDLE_POOL(HandleType::ShaderHandleT);
#undef  MAKE_HANDLE_POOL
	}

	~RendererStorage()
	{
#define DESTROY_HANDLE_POOL( TYPE ) W_DELETE(handles_[ TYPE ], handle_arena_)
		DESTROY_HANDLE_POOL(HandleType::ShaderHandleT);
		DESTROY_HANDLE_POOL(HandleType::TextureHandleT);
		DESTROY_HANDLE_POOL(HandleType::ShaderStorageBufferHandleT);
		DESTROY_HANDLE_POOL(HandleType::UniformBufferHandleT);
		DESTROY_HANDLE_POOL(HandleType::VertexArrayHandleT);
		DESTROY_HANDLE_POOL(HandleType::VertexBufferHandleT);
		DESTROY_HANDLE_POOL(HandleType::VertexBufferLayoutHandleT);
		DESTROY_HANDLE_POOL(HandleType::IndexBufferHandleT);
#undef  DESTROY_HANDLE_POOL
	}

	HandlePool* handles_[HandleType::Count];

	WRef<IndexBuffer>         index_buffers[k_max_handles[HandleType::IndexBufferHandleT]];
	WRef<BufferLayout>        vertex_buffer_layouts[k_max_handles[HandleType::VertexBufferLayoutHandleT]];
	WRef<VertexBuffer>        vertex_buffers[k_max_handles[HandleType::VertexBufferHandleT]];
	WRef<VertexArray>         vertex_arrays[k_max_handles[HandleType::VertexArrayHandleT]];
	WRef<UniformBuffer>       uniform_buffers[k_max_handles[HandleType::UniformBufferHandleT]];
	WRef<ShaderStorageBuffer> shader_storage_buffers[k_max_handles[HandleType::ShaderStorageBufferHandleT]];

	std::vector<CommandQueue> queues_;

	memory::HeapArea renderer_memory;
	LinearArena handle_arena_;
};
std::unique_ptr<RendererStorage> s_storage;

#define MAKE_VALIDATOR( HANDLE ) bool is_valid( HANDLE handle) { return s_storage->handles_[HandleType::HANDLE##T]->is_valid(handle.index); }
		MAKE_VALIDATOR(IndexBufferHandle);
		MAKE_VALIDATOR(VertexBufferLayoutHandle);
		MAKE_VALIDATOR(VertexBufferHandle);
		MAKE_VALIDATOR(VertexArrayHandle);
		MAKE_VALIDATOR(UniformBufferHandle);
		MAKE_VALIDATOR(ShaderStorageBufferHandle);
		MAKE_VALIDATOR(TextureHandle);
		MAKE_VALIDATOR(ShaderHandle);
#undef  MAKE_VALIDATOR


void MasterRenderer::init()
{
	DLOGN("render") << "[MasterRenderer] Allocating renderer storage." << std::endl;
	// Create storage object
	s_storage = std::make_unique<RendererStorage>();
	// Create command queues
	for(int queue_name = 0; queue_name < CommandQueue::Count; ++queue_name)
		s_storage->queues_.emplace_back(s_storage->renderer_memory.require_block(512_kB), // For commands
										s_storage->renderer_memory.require_block(2_MB));  // For auxiliary data
	DLOGI << "done" << std::endl;
}

void MasterRenderer::shutdown()
{
	DLOGN("render") << "[MasterRenderer] Releasing renderer storage." << std::endl;
	RendererStorage* rs = s_storage.release();
	delete rs;
	DLOGI << "done" << std::endl;
}

CommandQueue& MasterRenderer::get_queue(int name)
{
	W_ASSERT(name < CommandQueue::Count, "Unknown queue name!");
	return s_storage->queues_[name];
}

void MasterRenderer::flush()
{
	// Sort and flush each queue
	for(int queue_name = 0; queue_name < CommandQueue::Count; ++queue_name)
	{
		auto& queue = s_storage->queues_[queue_name];
		queue.sort();
		queue.flush();
		queue.reset();
	}
}

void MasterRenderer::test()
{
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(s_storage->renderer_memory.begin())+k_handle_alloc_size, 512_B);
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(s_storage->renderer_memory.begin())+k_handle_alloc_size+512_kB, 512_B);
}

/*
		   _____                                          _     
		  / ____|                                        | |    
		 | |     ___  _ __ ___  _ __ ___   __ _ _ __   __| |___ 
		 | |    / _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` / __|
		 | |___| (_) | | | | | | | | | | | (_| | | | | (_| \__ \
		  \_____\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_|___/
*/

CommandQueue::CommandQueue(std::pair<void*,void*> mem_range, std::pair<void*,void*> aux_mem_range):
command_buffer_(mem_range),
auxiliary_arena_(aux_mem_range),
count_(0)
{

}

CommandQueue::~CommandQueue()
{

}

void CommandQueue::reset()
{
	command_buffer_.reset();
	auxiliary_arena_.get_allocator().reset();
	count_ = 0;
}

IndexBufferHandle CommandQueue::create_index_buffer(uint64_t key, uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode)
{
	void* cmd = command_buffer_.get_head();
	IndexBufferHandle handle = { s_storage->handles_[IndexBufferHandleT]->acquire() };

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
	VertexBufferLayoutHandle handle = { s_storage->handles_[VertexBufferLayoutHandleT]->acquire() };

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
	VertexBufferHandle handle = { s_storage->handles_[VertexBufferHandleT]->acquire() };

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
	VertexArrayHandle handle = { s_storage->handles_[VertexArrayHandleT]->acquire() };

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
	UniformBufferHandle handle = { s_storage->handles_[UniformBufferHandleT]->acquire() };

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
	ShaderStorageBufferHandle handle = { s_storage->handles_[ShaderStorageBufferHandleT]->acquire() };

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
	s_storage->handles_[IndexBufferHandleT]->release(handle.index);

	void* cmd = command_buffer_.get_head();
	
	RenderCommand type = RenderCommand::DestroyIndexBuffer;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);

	push(cmd, key);
}

void CommandQueue::destroy_vertex_buffer_layout(uint64_t key, VertexBufferLayoutHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid VertexBufferLayoutHandle!");
	s_storage->handles_[VertexBufferLayoutHandleT]->release(handle.index);
	void* cmd = command_buffer_.get_head();
	
	RenderCommand type = RenderCommand::DestroyVertexBufferLayout;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);

	push(cmd, key);
}

void CommandQueue::destroy_vertex_buffer(uint64_t key, VertexBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid VertexBufferHandle!");
	s_storage->handles_[VertexBufferHandleT]->release(handle.index);
	void* cmd = command_buffer_.get_head();
	
	RenderCommand type = RenderCommand::DestroyVertexBuffer;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);

	push(cmd, key);
}

void CommandQueue::destroy_vertex_array(uint64_t key, VertexArrayHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid VertexArrayHandle!");
	s_storage->handles_[VertexArrayHandleT]->release(handle.index);
	void* cmd = command_buffer_.get_head();
	
	RenderCommand type = RenderCommand::DestroyVertexArray;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);

	push(cmd, key);
}

void CommandQueue::destroy_uniform_buffer(uint64_t key, UniformBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid UniformBufferHandle!");
	s_storage->handles_[UniformBufferHandleT]->release(handle.index);
	void* cmd = command_buffer_.get_head();
	
	RenderCommand type = RenderCommand::DestroyUniformBuffer;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);

	push(cmd, key);
}

void CommandQueue::destroy_shader_storage_buffer(uint64_t key, ShaderStorageBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid ShaderStorageBufferHandle!");
	s_storage->handles_[ShaderStorageBufferHandleT]->release(handle.index);
	void* cmd = command_buffer_.get_head();
	
	RenderCommand type = RenderCommand::DestroyShaderStorageBuffer;
	command_buffer_.write(&type);
	command_buffer_.write(&handle);

	push(cmd, key);
}

/*
		  _____  _                 _       _     
		 |  __ \(_)               | |     | |    
		 | |  | |_ ___ _ __   __ _| |_ ___| |__  
		 | |  | | / __| '_ \ / _` | __/ __| '_ \ 
		 | |__| | \__ \ |_) | (_| | || (__| | | |
		 |_____/|_|___/ .__/ \__,_|\__\___|_| |_|
		              | |                        
		              |_|                        
*/

namespace dispatch
{

void create_index_buffer(memory::LinearBuffer<>& buf)
{
	IndexBufferHandle handle;
	uint32_t count;
	DrawPrimitive primitive;
	DrawMode mode;
	uint32_t* auxiliary;

	buf.read(&handle);
	buf.read(&count);
	buf.read(&primitive);
	buf.read(&mode);
	buf.read(&auxiliary);

	s_storage->index_buffers[handle.index] = IndexBuffer::create(auxiliary, count, primitive, mode);
}

void create_vertex_buffer_layout(memory::LinearBuffer<>& buf)
{
	uint32_t count;
	VertexBufferLayoutHandle handle;
	BufferLayoutElement* auxiliary;
	buf.read(&handle);
	buf.read(&count);
	buf.read(&auxiliary);

	s_storage->vertex_buffer_layouts[handle.index] = make_ref<BufferLayout>(auxiliary, count);
}

void create_vertex_buffer(memory::LinearBuffer<>& buf)
{
	VertexBufferHandle handle;
	VertexBufferLayoutHandle layout_hnd;
	uint32_t count;
	DrawMode mode;
	float* auxiliary;
	buf.read(&handle);
	buf.read(&layout_hnd);
	buf.read(&count);
	buf.read(&mode);
	buf.read(&auxiliary);

	const auto& layout = *s_storage->vertex_buffer_layouts[layout_hnd.index];
	s_storage->vertex_buffers[handle.index] = VertexBuffer::create(auxiliary, count, layout, mode);
}

void create_vertex_array(memory::LinearBuffer<>& buf)
{
	VertexArrayHandle handle;
	VertexBufferHandle vb;
	IndexBufferHandle ib;
	buf.read(&handle);
	buf.read(&vb);
	buf.read(&ib);

	s_storage->vertex_arrays[handle.index] = VertexArray::create();
	s_storage->vertex_arrays[handle.index]->set_vertex_buffer(s_storage->vertex_buffers[vb.index]);
	if(is_valid(ib))
		s_storage->vertex_arrays[handle.index]->set_index_buffer(s_storage->index_buffers[ib.index]);
}

void create_uniform_buffer(memory::LinearBuffer<>& buf)
{
	UniformBufferHandle handle;
	uint32_t size;
	DrawMode mode;
	std::string name;
	uint8_t* auxiliary;
	buf.read(&handle);
	buf.read(&size);
	buf.read(&mode);
	buf.read_str(name);
	buf.read(&auxiliary);

	s_storage->uniform_buffers[handle.index] = UniformBuffer::create(name, auxiliary, size, mode);
}

void create_shader_storage_buffer(memory::LinearBuffer<>& buf)
{
	ShaderStorageBufferHandle handle;
	uint32_t size;
	DrawMode mode;
	std::string name;
	uint8_t* auxiliary;
	buf.read(&handle);
	buf.read(&size);
	buf.read(&mode);
	buf.read_str(name);
	buf.read(&auxiliary);

	s_storage->shader_storage_buffers[handle.index] = ShaderStorageBuffer::create(name, auxiliary, size, mode);
}

void update_index_buffer(memory::LinearBuffer<>& buf)
{
	IndexBufferHandle handle;
	uint32_t count;
	uint32_t* auxiliary;
	buf.read(&handle);
	buf.read(&count);
	buf.read(&auxiliary);

	s_storage->index_buffers[handle.index]->map(auxiliary, count);
}

void update_vertex_buffer(memory::LinearBuffer<>& buf)
{
	VertexBufferHandle handle;
	uint32_t size;
	uint8_t* auxiliary;
	buf.read(&handle);
	buf.read(&size);
	buf.read(&auxiliary);

	s_storage->vertex_buffers[handle.index]->map(auxiliary, size);
}

void update_uniform_buffer(memory::LinearBuffer<>& buf)
{
	UniformBufferHandle handle;
	uint32_t size;
	uint8_t* auxiliary;
	buf.read(&handle);
	buf.read(&size);
	buf.read(&auxiliary);

	s_storage->uniform_buffers[handle.index]->map(auxiliary);
}

void update_shader_storage_buffer(memory::LinearBuffer<>& buf)
{
	ShaderStorageBufferHandle handle;
	uint32_t size;
	uint8_t* auxiliary;
	buf.read(&handle);
	buf.read(&size);
	buf.read(&auxiliary);

	s_storage->shader_storage_buffers[handle.index]->map(auxiliary, size);
}

void destroy_index_buffer(memory::LinearBuffer<>& buf)
{
	IndexBufferHandle handle;
	buf.read(&handle);
	s_storage->index_buffers[handle.index] = nullptr;
}

void destroy_vertex_buffer_layout(memory::LinearBuffer<>& buf)
{
	VertexBufferLayoutHandle handle;
	buf.read(&handle);
	s_storage->vertex_buffer_layouts[handle.index] = nullptr;
}

void destroy_vertex_buffer(memory::LinearBuffer<>& buf)
{
	VertexBufferHandle handle;
	buf.read(&handle);
	s_storage->vertex_buffers[handle.index] = nullptr;
}

void destroy_vertex_array(memory::LinearBuffer<>& buf)
{
	VertexArrayHandle handle;
	buf.read(&handle);
	s_storage->vertex_arrays[handle.index] = nullptr;
}

void destroy_uniform_buffer(memory::LinearBuffer<>& buf)
{
	UniformBufferHandle handle;
	buf.read(&handle);
	s_storage->uniform_buffers[handle.index] = nullptr;
}

void destroy_shader_storage_buffer(memory::LinearBuffer<>& buf)
{
	ShaderStorageBufferHandle handle;
	buf.read(&handle);
	s_storage->shader_storage_buffers[handle.index] = nullptr;
}

} // namespace dispatch

typedef void (* backend_dispatch_func_t)(memory::LinearBuffer<>&);
static backend_dispatch_func_t backend_dispatch[(std::size_t)RenderCommand::Count] =
{
	&dispatch::create_index_buffer,
	&dispatch::create_vertex_buffer_layout,
	&dispatch::create_vertex_buffer,
	&dispatch::create_vertex_array,
	&dispatch::create_uniform_buffer,
	&dispatch::create_shader_storage_buffer,
	&dispatch::update_index_buffer,
	&dispatch::update_vertex_buffer,
	&dispatch::update_uniform_buffer,
	&dispatch::update_shader_storage_buffer,
	&dispatch::destroy_index_buffer,
	&dispatch::destroy_vertex_buffer_layout,
	&dispatch::destroy_vertex_buffer,
	&dispatch::destroy_vertex_array,
	&dispatch::destroy_uniform_buffer,
	&dispatch::destroy_shader_storage_buffer,
};

void CommandQueue::flush()
{
	for(int ii=0; ii<count_; ++ii)
	{
		auto&& [key,cmd] = commands_[ii];
		// TODO: handle state
		command_buffer_.seek(cmd);
		uint16_t type;
		command_buffer_.read(&type);
		(*backend_dispatch[type])(command_buffer_);
	}
}

} // namespace WIP
} // namespace erwin