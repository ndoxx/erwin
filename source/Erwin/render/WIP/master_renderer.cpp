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
		  _  __              
		 | |/ /              
		 | ' / ___ _   _ ___ 
		 |  < / _ \ | | / __|
		 | . \  __/ |_| \__ \
		 |_|\_\___|\__, |___/
		            __/ |    
		           |___/     
*/

// Sorting key system
constexpr uint8_t  k_view_bits       = 8;
constexpr uint8_t  k_draw_bits       = 1;
constexpr uint8_t  k_draw_type_bits  = 2;
constexpr uint8_t  k_transp_bits     = 1;
constexpr uint8_t  k_shader_bits     = 8;
constexpr uint8_t  k_depth_bits      = 32;
constexpr uint8_t  k_seq_bits        = 32;
constexpr uint64_t k_view_shift      = uint8_t(64)       - k_view_bits;
constexpr uint64_t k_draw_bit_shift  = k_view_shift      - k_draw_bits;
constexpr uint64_t k_draw_type_shift = k_draw_bit_shift  - k_draw_type_bits;
constexpr uint64_t k_1_transp_shift  = k_draw_type_shift - k_transp_bits;
constexpr uint64_t k_1_shader_shift  = k_1_transp_shift  - k_shader_bits;
constexpr uint64_t k_1_depth_shift   = k_1_shader_shift  - k_depth_bits;
constexpr uint64_t k_2_depth_shift   = k_draw_type_shift - k_depth_bits;
constexpr uint64_t k_2_transp_shift  = k_2_depth_shift   - k_transp_bits;
constexpr uint64_t k_2_shader_shift  = k_2_transp_shift  - k_shader_bits;
constexpr uint64_t k_3_seq_shift     = k_draw_type_shift - k_seq_bits;
constexpr uint64_t k_3_transp_shift  = k_3_seq_shift     - k_transp_bits;
constexpr uint64_t k_3_shader_shift  = k_3_transp_shift  - k_shader_bits;
constexpr uint64_t k_view_mask       = uint64_t(0x0000000f) << k_view_shift;
constexpr uint64_t k_draw_bit_mask   = uint64_t(0x00000001) << k_draw_bit_shift;
constexpr uint64_t k_draw_type_mask  = uint64_t(0x00000003) << k_draw_type_shift;
constexpr uint64_t k_1_transp_mask   = uint64_t(0x00000001) << k_1_transp_shift;
constexpr uint64_t k_1_shader_mask   = uint64_t(0x000000ff) << k_1_shader_shift;
constexpr uint64_t k_1_depth_mask    = uint64_t(0xffffffff) << k_1_depth_shift;
constexpr uint64_t k_2_depth_mask    = uint64_t(0xffffffff) << k_2_depth_shift;
constexpr uint64_t k_2_transp_mask   = uint64_t(0x00000001) << k_2_transp_shift;
constexpr uint64_t k_2_shader_mask   = uint64_t(0x000000ff) << k_2_shader_shift;
constexpr uint64_t k_3_seq_mask      = uint64_t(0xffffffff) << k_3_seq_shift;
constexpr uint64_t k_3_transp_mask   = uint64_t(0x00000001) << k_3_transp_shift;
constexpr uint64_t k_3_shader_mask   = uint64_t(0x000000ff) << k_3_shader_shift;

struct SortKey
{
	enum Order: uint8_t
	{
		ByShader,
		ByDepth,
		Sequential
	};

	uint64_t encode(Order type) const;
	void decode(uint64_t key);

						  // -- dependencies --     -- meaning --
	uint8_t view;         // queue global state?	layer / viewport id
	uint8_t transparency; // queue type 			blending type: opaque / transparent
	uint8_t shader;       // command data / type    could mean "material ID" when I have a material system
	bool is_draw;         // command data / type 	whether or not the command performs a draw call
	uint32_t depth;       // command data / type 	depth mantissa
	uint32_t sequence;    // command data / type 	for commands to be dispatched sequentially
};

uint64_t SortKey::encode(Order type) const
{
	uint64_t head = ((uint64_t(view)     << k_view_shift     ) & k_view_mask)
				  | ((uint64_t(~is_draw) << k_draw_bit_shift ) & k_draw_bit_mask)
				  | ((uint64_t(0)	   	 << k_draw_type_shift) & k_draw_type_mask);

	uint64_t body = 0;
	switch(type)
	{
		case Order::ByShader:
		{
			body |= ((uint64_t(transparency) << k_1_transp_shift) & k_1_transp_mask)
				 |  ((uint64_t(shader)       << k_1_shader_shift) & k_1_shader_mask)
				 |  ((uint64_t(depth)        << k_1_depth_shift)  & k_1_depth_mask);
			break;
		}
		case Order::ByDepth:
		{
			body |= ((uint64_t(depth)        << k_2_depth_shift)  & k_2_depth_mask)
				 |  ((uint64_t(transparency) << k_2_transp_shift) & k_2_transp_mask)
				 |  ((uint64_t(shader)       << k_2_shader_shift) & k_2_shader_mask);
			break;
		}
		case Order::Sequential:
		{
			body |= ((uint64_t(sequence)     << k_3_seq_shift)    & k_3_seq_mask)
				 |  ((uint64_t(transparency) << k_3_transp_shift) & k_3_transp_mask)
				 |  ((uint64_t(shader)       << k_3_shader_shift) & k_3_shader_mask);
			break;
		}
	}

	return head | body;
}

void SortKey::decode(uint64_t key)
{
	// TODO (if ever needed)
}


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
 										  						  + k_max_handles[HandleType::ShaderHandleT])
																  + HandleType::Count * sizeof(HandlePool)
																  + 128_B;	// TODO: compute actual size

struct RendererStorage
{
	RendererStorage():
	renderer_memory_(10_MB),
	handle_arena_(renderer_memory_.require_block(k_handle_alloc_size))
	{
		std::fill(std::begin(index_buffers),          std::end(index_buffers),          nullptr);
		std::fill(std::begin(vertex_buffer_layouts),  std::end(vertex_buffer_layouts),  nullptr);
		std::fill(std::begin(vertex_buffers),         std::end(vertex_buffers),         nullptr);
		std::fill(std::begin(vertex_arrays),          std::end(vertex_arrays),          nullptr);
		std::fill(std::begin(uniform_buffers),        std::end(uniform_buffers),        nullptr);
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

	// TODO: Drop WRefs and use a arenas (with pool allocator?) to allocate memory for these objects
	WRef<IndexBuffer>         index_buffers[k_max_handles[HandleType::IndexBufferHandleT]];
	WRef<BufferLayout>        vertex_buffer_layouts[k_max_handles[HandleType::VertexBufferLayoutHandleT]];
	WRef<VertexBuffer>        vertex_buffers[k_max_handles[HandleType::VertexBufferHandleT]];
	WRef<VertexArray>         vertex_arrays[k_max_handles[HandleType::VertexArrayHandleT]];
	WRef<UniformBuffer>       uniform_buffers[k_max_handles[HandleType::UniformBufferHandleT]];
	WRef<ShaderStorageBuffer> shader_storage_buffers[k_max_handles[HandleType::ShaderStorageBufferHandleT]];

	std::vector<CommandQueue> queues_;

	memory::HeapArea renderer_memory_;
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
		s_storage->queues_.emplace_back(s_storage->renderer_memory_);

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
		queue.flush(CommandQueue::Phase::Pre);
	}
	for(int queue_name = 0; queue_name < CommandQueue::Count; ++queue_name)
	{
		auto& queue = s_storage->queues_[queue_name];
		queue.flush(CommandQueue::Phase::Post);
		queue.reset();
	}
}

void MasterRenderer::DEBUG_test()
{
	const void* buf = s_storage->queues_[CommandQueue::QueueName::Resource].get_command_buffer();
	const void* aux = s_storage->queues_[CommandQueue::QueueName::Resource].get_auxiliary_buffer();
	memory::hex_dump(std::cout, buf, 512_B);
	memory::hex_dump(std::cout, aux, 512_B);
}

/*
		   _____                                          _     
		  / ____|                                        | |    
		 | |     ___  _ __ ___  _ __ ___   __ _ _ __   __| |___ 
		 | |    / _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` / __|
		 | |___| (_) | | | | | | | | | | | (_| | | | | (_| \__ \
		  \_____\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_|___/
*/

CommandQueue::CommandQueue(memory::HeapArea& memory):
command_buffer_(memory.require_block(512_kB)),
post_command_buffer_(memory.require_block(512_kB)),
auxiliary_arena_(memory.require_block(2_MB)),
count_(0),
post_count_(0)
{

}

CommandQueue::~CommandQueue()
{

}

void CommandQueue::sort()
{
	// Keys stored separately from commands to avoid touching data too
	// much during sort calls
    std::sort(std::begin(commands_), std::begin(commands_) + count_, 
        [&](const QueueItem& item1, const QueueItem& item2)
        {
        	return item1.first > item2.first;
        });
    std::sort(std::begin(post_commands_), std::begin(post_commands_) + post_count_, 
        [&](const QueueItem& item1, const QueueItem& item2)
        {
        	return item1.first > item2.first;
        });
}

void CommandQueue::reset()
{
	command_buffer_.reset();
	auxiliary_arena_.get_allocator().reset();
	count_ = 0;
	post_count_ = 0;
}

IndexBufferHandle CommandQueue::create_index_buffer(uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode)
{
	RenderCommand type = RenderCommand::CreateIndexBuffer;
	auto& cmdbuf = get_command_buffer(type);

	void* cmd = cmdbuf.head();
	IndexBufferHandle handle = { s_storage->handles_[IndexBufferHandleT]->acquire() };
	W_ASSERT(is_valid(handle), "No more free handle in handle pool.");

	// Write data
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&count);
	cmdbuf.write(&primitive);
	cmdbuf.write(&mode);

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
	cmdbuf.write(&auxiliary);

	SortKey key = { 0, 0, 0, false, 0, ~uint32_t(count_) };
	push(key.encode(SortKey::Sequential), type, cmd);
	return handle;
}

VertexBufferLayoutHandle CommandQueue::create_vertex_buffer_layout(const std::initializer_list<BufferLayoutElement>& elements)
{
	RenderCommand type = RenderCommand::CreateVertexBufferLayout;
	auto& cmdbuf = get_command_buffer(type);

	void* cmd = cmdbuf.head();
	VertexBufferLayoutHandle handle = { s_storage->handles_[VertexBufferLayoutHandleT]->acquire() };
	W_ASSERT(is_valid(handle), "No more free handle in handle pool.");

	std::vector<BufferLayoutElement> elts(elements);
	uint32_t count = elts.size();

	// Write data
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&count);

	// Write auxiliary data
	BufferLayoutElement* auxiliary = W_NEW_ARRAY_DYNAMIC(BufferLayoutElement, elts.size(), auxiliary_arena_);
	memcpy(auxiliary, elts.data(), elts.size() * sizeof(BufferLayoutElement));
	cmdbuf.write(&auxiliary);

	SortKey key = { 0, 0, 0, false, 0, ~uint32_t(count_) };
	push(key.encode(SortKey::Sequential), type, cmd);
	return handle;
}

VertexBufferHandle CommandQueue::create_vertex_buffer(VertexBufferLayoutHandle layout, float* vertex_data, uint32_t count, DrawMode mode)
{
	W_ASSERT(is_valid(layout), "Invalid VertexBufferLayoutHandle!");

	RenderCommand type = RenderCommand::CreateVertexBuffer;
	auto& cmdbuf = get_command_buffer(type);

	void* cmd = cmdbuf.head();
	VertexBufferHandle handle = { s_storage->handles_[VertexBufferHandleT]->acquire() };
	W_ASSERT(is_valid(handle), "No more free handle in handle pool.");

	// Write data
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&layout);
	cmdbuf.write(&count);
	cmdbuf.write(&mode);

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
	cmdbuf.write(&auxiliary);

	SortKey key = { 0, 0, 0, false, 0, ~uint32_t(count_) };
	push(key.encode(SortKey::Sequential), type, cmd);
	return handle;
}

VertexArrayHandle CommandQueue::create_vertex_array(VertexBufferHandle vb, IndexBufferHandle ib)
{
	W_ASSERT(is_valid(vb), "Invalid VertexBufferHandle!");
	W_ASSERT(is_valid(ib), "Invalid IndexBufferHandle!");

	RenderCommand type = RenderCommand::CreateVertexArray;
	auto& cmdbuf = get_command_buffer(type);

	void* cmd = cmdbuf.head();
	VertexArrayHandle handle = { s_storage->handles_[VertexArrayHandleT]->acquire() };
	W_ASSERT(is_valid(handle), "No more free handle in handle pool.");

	// Write data
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&vb);
	cmdbuf.write(&ib);

	SortKey key = { 0, 0, 0, false, 0, ~uint32_t(count_) };
	push(key.encode(SortKey::Sequential), type, cmd);
	return handle;
}

UniformBufferHandle CommandQueue::create_uniform_buffer(const std::string& name, void* data, uint32_t size, DrawMode mode)
{
	W_ASSERT(name.size()<=20, "UBO layout name should be less than 20 characters long.");
	
	RenderCommand type = RenderCommand::CreateUniformBuffer;
	auto& cmdbuf = get_command_buffer(type);

	void* cmd = cmdbuf.head();
	UniformBufferHandle handle = { s_storage->handles_[UniformBufferHandleT]->acquire() };
	W_ASSERT(is_valid(handle), "No more free handle in handle pool.");

	// Write data
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&size);
	cmdbuf.write(&mode);
	cmdbuf.write_str(name);

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
	cmdbuf.write(&auxiliary);

	SortKey key = { 0, 0, 0, false, 0, ~uint32_t(count_) };
	push(key.encode(SortKey::Sequential), type, cmd);
	return handle;
}

ShaderStorageBufferHandle CommandQueue::create_shader_storage_buffer(const std::string& name, void* data, uint32_t size, DrawMode mode)
{
	W_ASSERT(name.size()<=20, "SSBO layout name should be less than 20 characters long.");

	RenderCommand type = RenderCommand::CreateShaderStorageBuffer;
	auto& cmdbuf = get_command_buffer(type);

	void* cmd = cmdbuf.head();
	ShaderStorageBufferHandle handle = { s_storage->handles_[ShaderStorageBufferHandleT]->acquire() };
	W_ASSERT(is_valid(handle), "No more free handle in handle pool.");

	// Write data
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&size);
	cmdbuf.write(&mode);
	cmdbuf.write_str(name);

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
	cmdbuf.write(&auxiliary);

	SortKey key = { 0, 0, 0, false, 0, ~uint32_t(count_) };
	push(key.encode(SortKey::Sequential), type, cmd);
	return handle;
}

void CommandQueue::update_index_buffer(IndexBufferHandle handle, uint32_t* data, uint32_t count)
{
	W_ASSERT(is_valid(handle), "Invalid IndexBufferHandle!");
	W_ASSERT(data, "No data!");

	RenderCommand type = RenderCommand::UpdateIndexBuffer;
	auto& cmdbuf = get_command_buffer(type);

	void* cmd = cmdbuf.head();
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&count);
	uint32_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint32_t, count, auxiliary_arena_);
	memcpy(auxiliary, data, count);
	cmdbuf.write(&auxiliary);

	SortKey key = { 0, 0, 0, false, 0, ~uint32_t(count_) };
	push(key.encode(SortKey::Sequential), type, cmd);
}

void CommandQueue::update_vertex_buffer(VertexBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(is_valid(handle), "Invalid VertexBufferHandle!");
	W_ASSERT(data, "No data!");

	RenderCommand type = RenderCommand::UpdateVertexBuffer;
	auto& cmdbuf = get_command_buffer(type);
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&size);
	uint8_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, auxiliary_arena_);
	memcpy(auxiliary, data, size);
	cmdbuf.write(&auxiliary);

	SortKey key = { 0, 0, 0, false, 0, ~uint32_t(count_) };
	push(key.encode(SortKey::Sequential), type, cmd);
}

void CommandQueue::update_uniform_buffer(UniformBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(is_valid(handle), "Invalid UniformBufferHandle!");
	W_ASSERT(data, "No data!");

	RenderCommand type = RenderCommand::UpdateUniformBuffer;
	auto& cmdbuf = get_command_buffer(type);
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&size);
	uint8_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, auxiliary_arena_);
	memcpy(auxiliary, data, size);
	cmdbuf.write(&auxiliary);

	SortKey key = { 0, 0, 0, false, 0, ~uint32_t(count_) };
	push(key.encode(SortKey::Sequential), type, cmd);
}

void CommandQueue::update_shader_storage_buffer(ShaderStorageBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(is_valid(handle), "Invalid ShaderStorageBufferHandle!");
	W_ASSERT(data, "No data!");

	RenderCommand type = RenderCommand::UpdateShaderStorageBuffer;
	auto& cmdbuf = get_command_buffer(type);
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&size);
	uint8_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, auxiliary_arena_);
	memcpy(auxiliary, data, size);
	cmdbuf.write(&auxiliary);

	SortKey key = { 0, 0, 0, false, 0, ~uint32_t(count_) };
	push(key.encode(SortKey::Sequential), type, cmd);
}

void CommandQueue::destroy_index_buffer(IndexBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid IndexBufferHandle!");
	s_storage->handles_[IndexBufferHandleT]->release(handle.index);

	RenderCommand type = RenderCommand::DestroyIndexBuffer;
	auto& cmdbuf = get_command_buffer(type);
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);

	SortKey key = { 0, 0, 0, false, 0, ~uint32_t(post_count_) };
	push(key.encode(SortKey::Sequential), type, cmd);
}

void CommandQueue::destroy_vertex_buffer_layout(VertexBufferLayoutHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid VertexBufferLayoutHandle!");
	s_storage->handles_[VertexBufferLayoutHandleT]->release(handle.index);

	RenderCommand type = RenderCommand::DestroyVertexBufferLayout;
	auto& cmdbuf = get_command_buffer(type);
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);

	SortKey key = { 0, 0, 0, false, 0, ~uint32_t(post_count_) };
	push(key.encode(SortKey::Sequential), type, cmd);
}

void CommandQueue::destroy_vertex_buffer(VertexBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid VertexBufferHandle!");
	s_storage->handles_[VertexBufferHandleT]->release(handle.index);

	RenderCommand type = RenderCommand::DestroyVertexBuffer;
	auto& cmdbuf = get_command_buffer(type);
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);

	SortKey key = { 0, 0, 0, false, 0, ~uint32_t(post_count_) };
	push(key.encode(SortKey::Sequential), type, cmd);
}

void CommandQueue::destroy_vertex_array(VertexArrayHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid VertexArrayHandle!");
	s_storage->handles_[VertexArrayHandleT]->release(handle.index);

	RenderCommand type = RenderCommand::DestroyVertexArray;
	auto& cmdbuf = get_command_buffer(type);
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);

	SortKey key = { 0, 0, 0, false, 0, ~uint32_t(post_count_) };
	push(key.encode(SortKey::Sequential), type, cmd);
}

void CommandQueue::destroy_uniform_buffer(UniformBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid UniformBufferHandle!");
	s_storage->handles_[UniformBufferHandleT]->release(handle.index);

	RenderCommand type = RenderCommand::DestroyUniformBuffer;
	auto& cmdbuf = get_command_buffer(type);
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);

	SortKey key = { 0, 0, 0, false, 0, ~uint32_t(post_count_) };
	push(key.encode(SortKey::Sequential), type, cmd);
}

void CommandQueue::destroy_shader_storage_buffer(ShaderStorageBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid ShaderStorageBufferHandle!");
	s_storage->handles_[ShaderStorageBufferHandleT]->release(handle.index);

	RenderCommand type = RenderCommand::DestroyShaderStorageBuffer;
	auto& cmdbuf = get_command_buffer(type);
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);

	SortKey key = { 0, 0, 0, false, 0, ~uint32_t(post_count_) };
	push(key.encode(SortKey::Sequential), type, cmd);
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

void nop(memory::LinearBuffer<>& buf) { }

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
static backend_dispatch_func_t backend_dispatch[(std::size_t)CommandQueue::RenderCommand::Count] =
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

	&dispatch::nop,

	&dispatch::destroy_index_buffer,
	&dispatch::destroy_vertex_buffer_layout,
	&dispatch::destroy_vertex_buffer,
	&dispatch::destroy_vertex_array,
	&dispatch::destroy_uniform_buffer,
	&dispatch::destroy_shader_storage_buffer,
};

void CommandQueue::flush(Phase phase)
{
	auto& cbuf = get_command_buffer(phase);
	auto* cmds = (phase == Phase::Pre) ? commands_ : post_commands_;
	uint32_t count = (phase == Phase::Pre) ? count_ : post_count_;

	for(int ii=0; ii<count; ++ii)
	{
		auto&& [key,cmd] = cmds[ii];
		// TODO: handle state
		cbuf.seek(cmd);
		uint16_t type;
		cbuf.read(&type);
		(*backend_dispatch[type])(cbuf);
	}
}

} // namespace WIP
} // namespace erwin