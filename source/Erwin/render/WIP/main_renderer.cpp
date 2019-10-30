#include "render/WIP/main_renderer.h"

#include <map>
#include <memory>
#include <iostream>

#include "render/render_device.h"
#include "render/buffer.h"
#include "render/shader.h"
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

uint64_t SortKey::encode(SortKey::Order type) const
{
	uint64_t head = ((uint64_t(view)     << k_view_shift     ) & k_view_mask)
				  | ((uint64_t(~is_draw) << k_draw_bit_shift ) & k_draw_bit_mask)
				  | ((uint64_t(0)	   	 << k_draw_type_shift) & k_draw_type_mask);

	uint64_t body = 0;
	switch(type)
	{
		case SortKey::Order::ByShader:
		{
			body |= ((uint64_t(transparency) << k_1_transp_shift) & k_1_transp_mask)
				 |  ((uint64_t(shader)       << k_1_shader_shift) & k_1_shader_mask)
				 |  ((uint64_t(depth)        << k_1_depth_shift)  & k_1_depth_mask);
			break;
		}
		case SortKey::Order::ByDepth:
		{
			body |= ((uint64_t(depth)        << k_2_depth_shift)  & k_2_depth_mask)
				 |  ((uint64_t(transparency) << k_2_transp_shift) & k_2_transp_mask)
				 |  ((uint64_t(shader)       << k_2_shader_shift) & k_2_shader_mask);
			break;
		}
		case SortKey::Order::Sequential:
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
	renderer_memory_(20_MB),
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

		for(auto& ref: index_buffers) ref = nullptr;
		for(auto& ref: vertex_buffer_layouts) ref = nullptr;
		for(auto& ref: vertex_buffers) ref = nullptr;
		for(auto& ref: vertex_arrays) ref = nullptr;
		for(auto& ref: uniform_buffers) ref = nullptr;
		for(auto& ref: shader_storage_buffers) ref = nullptr;
		for(auto& ref: textures) ref = nullptr;
		for(auto& ref: shaders) ref = nullptr;
	}

	HandlePool* handles_[HandleType::Count];

	// TODO: Drop WRefs and use arenas (with pool allocator?) to allocate memory for these objects
	WRef<IndexBuffer>         index_buffers[k_max_handles[HandleType::IndexBufferHandleT]];
	WRef<BufferLayout>        vertex_buffer_layouts[k_max_handles[HandleType::VertexBufferLayoutHandleT]];
	WRef<VertexBuffer>        vertex_buffers[k_max_handles[HandleType::VertexBufferHandleT]];
	WRef<VertexArray>         vertex_arrays[k_max_handles[HandleType::VertexArrayHandleT]];
	WRef<UniformBuffer>       uniform_buffers[k_max_handles[HandleType::UniformBufferHandleT]];
	WRef<ShaderStorageBuffer> shader_storage_buffers[k_max_handles[HandleType::ShaderStorageBufferHandleT]];
	WRef<Texture2D>			  textures[k_max_handles[HandleType::TextureHandleT]];
	WRef<Shader>			  shaders[k_max_handles[HandleType::ShaderHandleT]];

	std::vector<RenderQueue> queues_;
	std::map<hash_t, ShaderHandle> shader_names_;
	// RendererStateCache cache_;

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


void MainRenderer::init()
{
	DLOGN("render") << "[MainRenderer] Allocating renderer storage." << std::endl;
	// Create storage object
	s_storage = std::make_unique<RendererStorage>();
	// Create command queues
	for(int queue_name = 0; queue_name < QueueName::Count; ++queue_name)
		s_storage->queues_.emplace_back(s_storage->renderer_memory_, SortKey::Order::Sequential);

	DLOGI << "done" << std::endl;
}

void MainRenderer::shutdown()
{
	flush();
	DLOGN("render") << "[MainRenderer] Releasing renderer storage." << std::endl;
	RendererStorage* rs = s_storage.release();
	delete rs;
	DLOGI << "done" << std::endl;
}

RenderQueue& MainRenderer::get_queue(int name)
{
	W_ASSERT(name < QueueName::Count, "Unknown queue name!");
	return s_storage->queues_[name];
}

void MainRenderer::flush()
{
	// Sort and flush each queue
	for(int queue_name = 0; queue_name < QueueName::Count; ++queue_name)
	{
		auto& queue = s_storage->queues_[queue_name];
		queue.sort();
		Gfx::device->clear(ClearFlags::CLEAR_COLOR_FLAG); // TMP: flags will change for each queue
		queue.flush(RenderQueue::Phase::Pre);
	}
	for(int queue_name = 0; queue_name < QueueName::Count; ++queue_name)
	{
		auto& queue = s_storage->queues_[queue_name];
		queue.flush(RenderQueue::Phase::Post);
		queue.reset();
	}
}

void MainRenderer::DEBUG_test()
{
	const void* pre_buf = s_storage->queues_[QueueName::Opaque].get_pre_command_buffer_ptr();
	const void* post_buf = s_storage->queues_[QueueName::Opaque].get_post_command_buffer_ptr();
	const void* aux = s_storage->queues_[QueueName::Opaque].get_auxiliary_buffer_ptr();
	memory::hex_dump(std::cout, pre_buf, 256_B, "CMDBuf-PRE");
	memory::hex_dump(std::cout, post_buf, 256_B, "CMDBuf-POST");
	memory::hex_dump(std::cout, aux, 256_B, "AUX");
}

/*
		   _____                                          _     
		  / ____|                                        | |    
		 | |     ___  _ __ ___  _ __ ___   __ _ _ __   __| |___ 
		 | |    / _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` / __|
		 | |___| (_) | | | | | | | | | | | (_| | | | | (_| \__ \
		  \_____\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_|___/
*/

DrawCall::DrawCall(RenderQueue& queue, Type type, ShaderHandle shader, VertexArrayHandle VAO, uint32_t count, uint32_t offset):
type(type),
shader(shader),
VAO(VAO),
UBO_data(nullptr),
SSBO_data(nullptr),
UBO_size(0),
SSBO_size(0),
count(count),
offset(offset),
queue(queue)
{

}

RenderQueue::RenderQueue(memory::HeapArea& memory, SortKey::Order order):
order_(order),
key_({ 0, 0, 0, false, 0, 0 }),
pre_buffer_(memory.require_block(512_kB)),
post_buffer_(memory.require_block(512_kB)),
auxiliary_arena_(memory.require_block(2_MB))
{

}

RenderQueue::~RenderQueue()
{

}

void RenderQueue::set_clear_color(uint8_t R, uint8_t G, uint8_t B, uint8_t A)
{
	clear_color_ = (R << 0) + (G << 8) + (B << 16) + (A << 24);
	Gfx::device->set_clear_color(R/255.f, G/255.f, B/255.f, A/255.f);
}

void RenderQueue::set_state(const PassState& state)
{
	state_flags_ = state.encode();

	// TMP
	//Gfx::framebuffer_pool->bind(state.render_target);
	Gfx::device->set_cull_mode(state.rasterizer_state.cull_mode);
	
	if(state.blend_state == BlendState::Alpha)
		Gfx::device->set_std_blending();
	else
		Gfx::device->disable_blending();

	Gfx::device->set_stencil_test_enabled(state.depth_stencil_state.stencil_test_enabled);
	if(state.depth_stencil_state.stencil_test_enabled)
	{
		Gfx::device->set_stencil_func(state.depth_stencil_state.stencil_func);
		Gfx::device->set_stencil_operator(state.depth_stencil_state.stencil_operator);
	}

	Gfx::device->set_depth_test_enabled(state.depth_stencil_state.depth_test_enabled);
	if(state.depth_stencil_state.depth_test_enabled)
		Gfx::device->set_depth_func(state.depth_stencil_state.depth_func);
}

void RenderQueue::sort()
{
	// Keys stored separately from commands to avoid touching data too
	// much during sort calls
    std::sort(std::begin(pre_buffer_.entries), std::begin(pre_buffer_.entries) + pre_buffer_.count, 
        [&](const CommandBuffer::Entry& item1, const CommandBuffer::Entry& item2)
        {
        	return item1.first > item2.first;
        });
    std::sort(std::begin(post_buffer_.entries), std::begin(post_buffer_.entries) + post_buffer_.count, 
        [&](const CommandBuffer::Entry& item1, const CommandBuffer::Entry& item2)
        {
        	return item1.first > item2.first;
        });
}

void RenderQueue::push_command(RenderCommand type, void* cmd)
{
	auto& cmdbuf = get_command_buffer(type);
	key_.is_draw = (type == RenderCommand::Submit);
	key_.sequence = ~uint32_t(cmdbuf.count); // TODO: per-view sequence
	uint64_t key = key_.encode(order_);
	cmdbuf.entries[cmdbuf.count++] = {key, cmd};
}

void RenderQueue::reset()
{
	pre_buffer_.reset();
	post_buffer_.reset();
	auxiliary_arena_.get_allocator().reset();
	key_ = { 0, 0, 0, false, 0, 0 };
}

IndexBufferHandle RenderQueue::create_index_buffer(uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode)
{
	RenderCommand type = RenderCommand::CreateIndexBuffer;
	auto& cmdbuf = get_command_buffer(type).storage;

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

	push_command(type, cmd);
	return handle;
}

VertexBufferLayoutHandle RenderQueue::create_vertex_buffer_layout(const std::initializer_list<BufferLayoutElement>& elements)
{
	RenderCommand type = RenderCommand::CreateVertexBufferLayout;
	auto& cmdbuf = get_command_buffer(type).storage;

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

	push_command(type, cmd);
	return handle;
}

VertexBufferHandle RenderQueue::create_vertex_buffer(VertexBufferLayoutHandle layout, float* vertex_data, uint32_t count, DrawMode mode)
{
	W_ASSERT(is_valid(layout), "Invalid VertexBufferLayoutHandle!");

	RenderCommand type = RenderCommand::CreateVertexBuffer;
	auto& cmdbuf = get_command_buffer(type).storage;

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

	push_command(type, cmd);
	return handle;
}

VertexArrayHandle RenderQueue::create_vertex_array(VertexBufferHandle vb, IndexBufferHandle ib)
{
	W_ASSERT(is_valid(vb), "Invalid VertexBufferHandle!");
	W_ASSERT(is_valid(ib), "Invalid IndexBufferHandle!");

	RenderCommand type = RenderCommand::CreateVertexArray;
	auto& cmdbuf = get_command_buffer(type).storage;

	void* cmd = cmdbuf.head();
	VertexArrayHandle handle = { s_storage->handles_[VertexArrayHandleT]->acquire() };
	W_ASSERT(is_valid(handle), "No more free handle in handle pool.");

	// Write data
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&vb);
	cmdbuf.write(&ib);

	push_command(type, cmd);
	return handle;
}

UniformBufferHandle RenderQueue::create_uniform_buffer(const std::string& name, void* data, uint32_t size, DrawMode mode)
{
	RenderCommand type = RenderCommand::CreateUniformBuffer;
	auto& cmdbuf = get_command_buffer(type).storage;

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

	push_command(type, cmd);
	return handle;
}

ShaderStorageBufferHandle RenderQueue::create_shader_storage_buffer(const std::string& name, void* data, uint32_t size, DrawMode mode)
{
	RenderCommand type = RenderCommand::CreateShaderStorageBuffer;
	auto& cmdbuf = get_command_buffer(type).storage;

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

	push_command(type, cmd);
	return handle;
}

ShaderHandle RenderQueue::create_shader(const fs::path& filepath, const std::string& name)
{
	RenderCommand type = RenderCommand::CreateShader;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();
	ShaderHandle handle = { s_storage->handles_[ShaderHandleT]->acquire() };
	W_ASSERT(is_valid(handle), "No more free handle in handle pool.");

	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write_str(filepath.string());
	cmdbuf.write_str(name);

	push_command(type, cmd);
	return handle;
}

TextureHandle RenderQueue::create_texture_2D(const Texture2DDescriptor& desc)
{
	RenderCommand type = RenderCommand::CreateTexture2D;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();
	TextureHandle handle = { s_storage->handles_[TextureHandleT]->acquire() };
	W_ASSERT(is_valid(handle), "No more free handle in handle pool.");

	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&desc);

	push_command(type, cmd);
	return handle;
}

void RenderQueue::update_index_buffer(IndexBufferHandle handle, uint32_t* data, uint32_t count)
{
	W_ASSERT(is_valid(handle), "Invalid IndexBufferHandle!");
	W_ASSERT(data, "No data!");

	RenderCommand type = RenderCommand::UpdateIndexBuffer;
	auto& cmdbuf = get_command_buffer(type).storage;

	void* cmd = cmdbuf.head();
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&count);
	uint32_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint32_t, count, auxiliary_arena_);
	memcpy(auxiliary, data, count);
	cmdbuf.write(&auxiliary);

	push_command(type, cmd);
}

void RenderQueue::update_vertex_buffer(VertexBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(is_valid(handle), "Invalid VertexBufferHandle!");
	W_ASSERT(data, "No data!");

	RenderCommand type = RenderCommand::UpdateVertexBuffer;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&size);
	uint8_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, auxiliary_arena_);
	memcpy(auxiliary, data, size);
	cmdbuf.write(&auxiliary);

	push_command(type, cmd);
}

void RenderQueue::update_uniform_buffer(UniformBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(is_valid(handle), "Invalid UniformBufferHandle!");
	W_ASSERT(data, "No data!");

	RenderCommand type = RenderCommand::UpdateUniformBuffer;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&size);
	uint8_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, auxiliary_arena_);
	memcpy(auxiliary, data, size);
	cmdbuf.write(&auxiliary);

	push_command(type, cmd);
}

void RenderQueue::update_shader_storage_buffer(ShaderStorageBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(is_valid(handle), "Invalid ShaderStorageBufferHandle!");
	W_ASSERT(data, "No data!");

	RenderCommand type = RenderCommand::UpdateShaderStorageBuffer;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&size);
	uint8_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, auxiliary_arena_);
	memcpy(auxiliary, data, size);
	cmdbuf.write(&auxiliary);

	push_command(type, cmd);
}

void RenderQueue::shader_attach_uniform_buffer(ShaderHandle shader, UniformBufferHandle ubo)
{
	W_ASSERT(is_valid(shader), "Invalid ShaderHandle!");
	W_ASSERT(is_valid(ubo), "Invalid UniformBufferHandle!");

	RenderCommand type = RenderCommand::ShaderAttachUniformBuffer;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&shader);
	cmdbuf.write(&ubo);

	push_command(type, cmd);
}

void RenderQueue::shader_attach_storage_buffer(ShaderHandle shader, ShaderStorageBufferHandle ssbo)
{
	W_ASSERT(is_valid(shader), "Invalid ShaderHandle!");
	W_ASSERT(is_valid(ssbo), "Invalid ShaderStorageBufferHandle!");

	RenderCommand type = RenderCommand::ShaderAttachStorageBuffer;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&shader);
	cmdbuf.write(&ssbo);

	push_command(type, cmd);
}

void RenderQueue::submit(const DrawCall& dc)
{
	W_ASSERT(is_valid(dc.VAO), "Invalid VertexArrayHandle!");
	W_ASSERT(is_valid(dc.shader), "Invalid ShaderHandle!");

	RenderCommand type = RenderCommand::Submit;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();

	cmdbuf.write(&type);
	cmdbuf.write(&dc.type);
	cmdbuf.write(&dc.VAO);
	cmdbuf.write(&dc.shader);
	cmdbuf.write(&dc.UBO);
	cmdbuf.write(&dc.SSBO);
	cmdbuf.write(&dc.UBO_size);
	cmdbuf.write(&dc.SSBO_size);
	cmdbuf.write(&dc.count);
	cmdbuf.write(&dc.instance_count);
	cmdbuf.write(&dc.offset);
	cmdbuf.write(&dc.sampler);
	cmdbuf.write(&dc.texture);
	uint8_t* ubo_data = nullptr;
	uint8_t* ssbo_data = nullptr;
	if(dc.UBO_data)
	{
		ubo_data = W_NEW_ARRAY_DYNAMIC(uint8_t, dc.UBO_size, auxiliary_arena_);
		memcpy(ubo_data, dc.UBO_data, dc.UBO_size);
	}
	if(dc.SSBO_data)
	{
		ssbo_data = W_NEW_ARRAY_DYNAMIC(uint8_t, dc.SSBO_size, auxiliary_arena_);
		memcpy(ssbo_data, dc.SSBO_data, dc.SSBO_size);
	}
	cmdbuf.write(&ubo_data);
	cmdbuf.write(&ssbo_data);

	push_command(type, cmd);
}

void RenderQueue::destroy_index_buffer(IndexBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid IndexBufferHandle!");
	s_storage->handles_[IndexBufferHandleT]->release(handle.index);

	RenderCommand type = RenderCommand::DestroyIndexBuffer;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);

	push_command(type, cmd);
}

void RenderQueue::destroy_vertex_buffer_layout(VertexBufferLayoutHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid VertexBufferLayoutHandle!");
	s_storage->handles_[VertexBufferLayoutHandleT]->release(handle.index);

	RenderCommand type = RenderCommand::DestroyVertexBufferLayout;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);

	push_command(type, cmd);
}

void RenderQueue::destroy_vertex_buffer(VertexBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid VertexBufferHandle!");
	s_storage->handles_[VertexBufferHandleT]->release(handle.index);

	RenderCommand type = RenderCommand::DestroyVertexBuffer;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);

	push_command(type, cmd);
}

void RenderQueue::destroy_vertex_array(VertexArrayHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid VertexArrayHandle!");
	s_storage->handles_[VertexArrayHandleT]->release(handle.index);

	RenderCommand type = RenderCommand::DestroyVertexArray;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);

	push_command(type, cmd);
}

void RenderQueue::destroy_uniform_buffer(UniformBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid UniformBufferHandle!");
	s_storage->handles_[UniformBufferHandleT]->release(handle.index);

	RenderCommand type = RenderCommand::DestroyUniformBuffer;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);

	push_command(type, cmd);
}

void RenderQueue::destroy_shader_storage_buffer(ShaderStorageBufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid ShaderStorageBufferHandle!");
	s_storage->handles_[ShaderStorageBufferHandleT]->release(handle.index);

	RenderCommand type = RenderCommand::DestroyShaderStorageBuffer;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);

	push_command(type, cmd);
}

void RenderQueue::destroy_shader(ShaderHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid ShaderHandle!");
	s_storage->handles_[ShaderHandleT]->release(handle.index);
	
	RenderCommand type = RenderCommand::DestroyShader;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();

	cmdbuf.write(&type);
	cmdbuf.write(&handle);

	push_command(type, cmd);
}

void RenderQueue::destroy_texture_2D(TextureHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid TextureHandle!");
	s_storage->handles_[TextureHandleT]->release(handle.index);
	
	RenderCommand type = RenderCommand::DestroyTexture2D;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();

	cmdbuf.write(&type);
	cmdbuf.write(&handle);

	push_command(type, cmd);
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

void create_shader(memory::LinearBuffer<>& buf)
{
	ShaderHandle handle;
	std::string filepath;
	std::string name;
	buf.read(&handle);
	buf.read_str(filepath);
	buf.read_str(name);

	hash_t hname = H_(name.c_str());
	W_ASSERT(s_storage->shader_names_.find(hname)==s_storage->shader_names_.end(), "Shader already loaded.");

	s_storage->shaders[handle.index] = Shader::create(name, fs::path(filepath));
	s_storage->shader_names_.insert(std::pair(hname, handle));
}

void create_texture_2D(memory::LinearBuffer<>& buf)
{
	TextureHandle handle;
	Texture2DDescriptor descriptor;
	buf.read(&handle);
	buf.read(&descriptor);

	s_storage->textures[handle.index] = Texture2D::create(descriptor);
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

void shader_attach_uniform_buffer(memory::LinearBuffer<>& buf)
{
	ShaderHandle shader_handle;
	UniformBufferHandle ubo_handle;
	buf.read(&shader_handle);
	buf.read(&ubo_handle);

	auto& shader = *s_storage->shaders[shader_handle.index];
	auto& ubo = *s_storage->uniform_buffers[ubo_handle.index];

	shader.attach_uniform_buffer(ubo);
}

void shader_attach_storage_buffer(memory::LinearBuffer<>& buf)
{
	ShaderHandle shader_handle;
	ShaderStorageBufferHandle ssbo_handle;
	buf.read(&shader_handle);
	buf.read(&ssbo_handle);

	auto& shader = *s_storage->shaders[shader_handle.index];
	auto& ssbo = *s_storage->shader_storage_buffers[ssbo_handle.index];

	shader.attach_shader_storage(ssbo);
}

void submit(memory::LinearBuffer<>& buf)
{
	DrawCall::Type type;
	VertexArrayHandle va_handle;
	ShaderHandle shader_handle;
	UniformBufferHandle ubo_handle;
	ShaderStorageBufferHandle ssbo_handle;
	TextureHandle texture_handle;
	hash_t sampler;
	uint32_t count;
	uint32_t instance_count;
	uint32_t offset;
	uint32_t ubo_size;
	uint32_t ssbo_size;
	uint8_t* ubo_data;
	uint8_t* ssbo_data;

	buf.read(&type);
	buf.read(&va_handle);
	buf.read(&shader_handle);
	buf.read(&ubo_handle);
	buf.read(&ssbo_handle);
	buf.read(&ubo_size);
	buf.read(&ssbo_size);
	buf.read(&count);
	buf.read(&instance_count);
	buf.read(&offset);
	buf.read(&sampler);
	buf.read(&texture_handle);
	buf.read(&ubo_data);
	buf.read(&ssbo_data);

	auto& va = *s_storage->vertex_arrays[va_handle.index];
	auto& shader = *s_storage->shaders[shader_handle.index];

	shader.bind();

	if(ubo_data)
	{
		auto& ubo = *s_storage->uniform_buffers[ubo_handle.index];
		ubo.stream(ubo_data, ubo_size, 0);
		// shader.attach_uniform_buffer(ubo);
	}
	if(ssbo_data)
	{
		auto& ssbo = *s_storage->shader_storage_buffers[ssbo_handle.index];
		ssbo.stream(ssbo_data, ssbo_size, 0);
		// shader.attach_shader_storage(ssbo);
	}
	if(is_valid(texture_handle))
	{
		auto& texture = *s_storage->textures[texture_handle.index];
		shader.attach_texture(sampler, texture);
	}

	switch(type)
	{
		case DrawCall::Indexed:
			Gfx::device->draw_indexed(va, count, offset);
			break;
		case DrawCall::IndexedInstanced:
			Gfx::device->draw_indexed_instanced(va, instance_count);
			break;
		default:
			break;
	}
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

void destroy_shader(memory::LinearBuffer<>& buf)
{
	ShaderHandle handle;
	buf.read(&handle);
	hash_t hname = H_(s_storage->shaders[handle.index]->get_name().c_str());
	s_storage->shaders[handle.index] = nullptr;
	s_storage->shader_names_.erase(hname);
}

void destroy_texture_2D(memory::LinearBuffer<>& buf)
{
	TextureHandle handle;
	buf.read(&handle);
	s_storage->textures[handle.index] = nullptr;
}

} // namespace dispatch

typedef void (* backend_dispatch_func_t)(memory::LinearBuffer<>&);
static backend_dispatch_func_t backend_dispatch[(std::size_t)RenderQueue::RenderCommand::Count] =
{
	&dispatch::create_index_buffer,
	&dispatch::create_vertex_buffer_layout,
	&dispatch::create_vertex_buffer,
	&dispatch::create_vertex_array,
	&dispatch::create_uniform_buffer,
	&dispatch::create_shader_storage_buffer,
	&dispatch::create_shader,
	&dispatch::create_texture_2D,
	&dispatch::update_index_buffer,
	&dispatch::update_vertex_buffer,
	&dispatch::update_uniform_buffer,
	&dispatch::update_shader_storage_buffer,
	&dispatch::shader_attach_uniform_buffer,
	&dispatch::shader_attach_storage_buffer,
	&dispatch::submit,

	&dispatch::nop,

	&dispatch::destroy_index_buffer,
	&dispatch::destroy_vertex_buffer_layout,
	&dispatch::destroy_vertex_buffer,
	&dispatch::destroy_vertex_array,
	&dispatch::destroy_uniform_buffer,
	&dispatch::destroy_shader_storage_buffer,
	&dispatch::destroy_shader,
	&dispatch::destroy_texture_2D,
};

void RenderQueue::flush(Phase phase)
{
	auto& cbuf = get_command_buffer(phase);

	for(int ii=0; ii<cbuf.count; ++ii)
	{
		auto&& [key,cmd] = cbuf.entries[ii];
		// TODO: handle state
		cbuf.storage.seek(cmd);
		uint16_t type;
		cbuf.storage.read(&type);
		(*backend_dispatch[type])(cbuf.storage);
	}
}

} // namespace WIP
} // namespace erwin