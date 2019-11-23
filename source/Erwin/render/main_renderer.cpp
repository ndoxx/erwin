#include "render/main_renderer.h"

#include <map>
#include <memory>
#include <iostream>

#include "render/render_device.h"
#include "render/buffer.h"
#include "render/framebuffer.h"
#include "render/shader.h"
#include "render/query_timer.h"
#include "debug/logger.h"
#include "memory/arena.h"
#include "memory/memory_utils.h"
#include "memory/handle_pool.h"
#include "memory/linear_allocator.h"
#include "memory/handle_pool.h"
#include "core/config.h"

namespace erwin
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
constexpr uint8_t  k_draw_type_bits  = 2;
constexpr uint8_t  k_shader_bits     = 8;
constexpr uint8_t  k_depth_bits      = 32;
constexpr uint8_t  k_seq_bits        = 32;
constexpr uint64_t k_view_shift      = uint8_t(64)       - k_view_bits;
constexpr uint64_t k_draw_type_shift = k_view_shift  - k_draw_type_bits;
constexpr uint64_t k_1_shader_shift  = k_draw_type_shift  - k_shader_bits;
constexpr uint64_t k_1_depth_shift   = k_1_shader_shift  - k_depth_bits;
constexpr uint64_t k_2_depth_shift   = k_draw_type_shift - k_depth_bits;
constexpr uint64_t k_2_shader_shift  = k_2_depth_shift  - k_shader_bits;
constexpr uint64_t k_3_seq_shift     = k_draw_type_shift - k_seq_bits;
constexpr uint64_t k_3_shader_shift  = k_3_seq_shift  - k_shader_bits;
constexpr uint64_t k_view_mask       = uint64_t(0x0000000f) << k_view_shift;
constexpr uint64_t k_draw_type_mask  = uint64_t(0x00000003) << k_draw_type_shift;
constexpr uint64_t k_1_shader_mask   = uint64_t(0x000000ff) << k_1_shader_shift;
constexpr uint64_t k_1_depth_mask    = uint64_t(0xffffffff) << k_1_depth_shift;
constexpr uint64_t k_2_depth_mask    = uint64_t(0xffffffff) << k_2_depth_shift;
constexpr uint64_t k_2_shader_mask   = uint64_t(0x000000ff) << k_2_shader_shift;
constexpr uint64_t k_3_seq_mask      = uint64_t(0xffffffff) << k_3_seq_shift;
constexpr uint64_t k_3_shader_mask   = uint64_t(0x000000ff) << k_3_shader_shift;

uint64_t SortKey::encode(SortKey::Order type) const
{
	uint64_t head = ((uint64_t(view)         << k_view_shift     ) & k_view_mask)
				  | ((uint64_t(0)	   	     << k_draw_type_shift) & k_draw_type_mask);

	uint64_t body = 0;
	switch(type)
	{
		case SortKey::Order::ByShader:
		{
			body |= ((uint64_t(shader)       << k_1_shader_shift) & k_1_shader_mask)
				 |  ((uint64_t(depth)        << k_1_depth_shift)  & k_1_depth_mask);
			break;
		}
		case SortKey::Order::ByDepthDescending:
		{
			body |= ((uint64_t(~depth)       << k_2_depth_shift)  & k_2_depth_mask)
				 |  ((uint64_t(shader)       << k_2_shader_shift) & k_2_shader_mask);
			break;
		}
		case SortKey::Order::ByDepthAscending:
		{
			body |= ((uint64_t(depth)        << k_2_depth_shift)  & k_2_depth_mask)
				 |  ((uint64_t(shader)       << k_2_shader_shift) & k_2_shader_mask);
			break;
		}
		case SortKey::Order::Sequential:
		{
			body |= ((uint64_t(sequence)     << k_3_seq_shift)    & k_3_seq_mask)
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

struct FramebufferTextureVector
{
	std::vector<TextureHandle> handles;
};

constexpr std::size_t k_handle_alloc_size = 2 * sizeof(uint16_t) * (k_max_handles[HandleType::IndexBufferHandleT]
 										  						  + k_max_handles[HandleType::VertexBufferLayoutHandleT]
 										  						  + k_max_handles[HandleType::VertexBufferHandleT]
 										  						  + k_max_handles[HandleType::VertexArrayHandleT]
 										  						  + k_max_handles[HandleType::UniformBufferHandleT]
 										  						  + k_max_handles[HandleType::ShaderStorageBufferHandleT]
 										  						  + k_max_handles[HandleType::TextureHandleT]
 										  						  + k_max_handles[HandleType::ShaderHandleT]
 										  						  + k_max_handles[HandleType::FramebufferHandleT])
																  + HandleType::Count * sizeof(HandlePool)
																  + 128_B;	// TODO: compute actual size

struct RendererStorage
{
	RendererStorage():
	renderer_memory_(cfg::get<size_t>("erwin.renderer.memory.renderer_area"_h, 20_MB)),
	pre_buffer_(renderer_memory_.require_block(cfg::get<size_t>("erwin.renderer.memory.pre_buffer"_h, 512_kB))),
	post_buffer_(renderer_memory_.require_block(cfg::get<size_t>("erwin.renderer.memory.post_buffer"_h, 512_kB))),
	auxiliary_arena_(renderer_memory_.require_block(cfg::get<size_t>("erwin.renderer.memory.auxiliary_arena"_h, 2_MB))),
	handle_arena_(renderer_memory_.require_block(k_handle_alloc_size))
	{
#ifdef W_DEBUG
		pre_buffer_.storage.set_debug_name("CB-Pre");
		post_buffer_.storage.set_debug_name("CB-Post");
		auxiliary_arena_.set_debug_name("Auxiliary");
#endif

		std::fill(std::begin(index_buffers),          std::end(index_buffers),          nullptr);
		std::fill(std::begin(vertex_buffer_layouts),  std::end(vertex_buffer_layouts),  nullptr);
		std::fill(std::begin(vertex_buffers),         std::end(vertex_buffers),         nullptr);
		std::fill(std::begin(vertex_arrays),          std::end(vertex_arrays),          nullptr);
		std::fill(std::begin(uniform_buffers),        std::end(uniform_buffers),        nullptr);
		std::fill(std::begin(shader_storage_buffers), std::end(shader_storage_buffers), nullptr);
		std::fill(std::begin(textures),               std::end(textures),               nullptr);
		std::fill(std::begin(shaders),                std::end(shaders),                nullptr);
		std::fill(std::begin(framebuffers),           std::end(framebuffers),           nullptr);

#define MAKE_HANDLE_POOL( TYPE ) handles_[ TYPE ] = W_NEW(HandlePoolT<k_max_handles[ TYPE ]>, handle_arena_)
		MAKE_HANDLE_POOL(HandleType::IndexBufferHandleT);
		MAKE_HANDLE_POOL(HandleType::VertexBufferLayoutHandleT);
		MAKE_HANDLE_POOL(HandleType::VertexBufferHandleT);
		MAKE_HANDLE_POOL(HandleType::VertexArrayHandleT);
		MAKE_HANDLE_POOL(HandleType::UniformBufferHandleT);
		MAKE_HANDLE_POOL(HandleType::ShaderStorageBufferHandleT);
		MAKE_HANDLE_POOL(HandleType::TextureHandleT);
		MAKE_HANDLE_POOL(HandleType::ShaderHandleT);
		MAKE_HANDLE_POOL(HandleType::FramebufferHandleT);
#undef  MAKE_HANDLE_POOL
	}

	~RendererStorage()
	{
#define DESTROY_HANDLE_POOL( TYPE ) W_DELETE(handles_[ TYPE ], handle_arena_)
		DESTROY_HANDLE_POOL(HandleType::FramebufferHandleT);
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
		for(auto& ref: framebuffers) ref = nullptr;
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
	WRef<Framebuffer>		  framebuffers[k_max_handles[HandleType::FramebufferHandleT]];

	FramebufferHandle default_framebuffer_;
	std::map<uint16_t, FramebufferTextureVector> framebuffer_textures_;

	std::map<uint32_t, RenderQueue> queues_;
	std::map<hash_t, ShaderHandle> shader_names_;

	WScope<QueryTimer> query_timer;
	bool profiling_enabled;
	MainRendererStats stats;

	memory::HeapArea renderer_memory_;
	CommandBuffer pre_buffer_;
	CommandBuffer post_buffer_;
	AuxArena auxiliary_arena_;
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
		MAKE_VALIDATOR(FramebufferHandle);
#undef  MAKE_VALIDATOR

void MainRenderer::init()
{
	DLOGN("render") << "[MainRenderer] Allocating renderer storage." << std::endl;
	
	// Create and initialize storage object
	s_storage = std::make_unique<RendererStorage>();
	s_storage->query_timer = QueryTimer::create();
	s_storage->profiling_enabled = false;
	s_storage->default_framebuffer_ = { s_storage->handles_[FramebufferHandleT]->acquire() };

	DLOGI << "done" << std::endl;
}

void MainRenderer::shutdown()
{
	flush();
	DLOGN("render") << "[MainRenderer] Releasing renderer storage." << std::endl;
	s_storage->handles_[FramebufferHandleT]->release(s_storage->default_framebuffer_.index);
	RendererStorage* rs = s_storage.release();
	delete rs;
	DLOGI << "done" << std::endl;
}

void MainRenderer::create_queue(uint32_t name, SortKey::Order order)
{
	s_storage->queues_.emplace(std::piecewise_construct,
              				   std::forward_as_tuple(name),
              				   std::forward_as_tuple(order, s_storage->renderer_memory_));
}

RenderQueue& MainRenderer::get_queue(uint32_t name)
{
	auto it = s_storage->queues_.find(name);
	W_ASSERT(it != s_storage->queues_.end(), "Unknown queue name!");
	return it->second;
}

AuxArena& MainRenderer::get_arena()
{
	return s_storage->auxiliary_arena_;
}

void MainRenderer::set_profiling_enabled(bool value)
{
	s_storage->profiling_enabled = value;
}

const MainRendererStats& MainRenderer::get_stats()
{
	return s_storage->stats;
}

FramebufferHandle MainRenderer::default_render_target()
{
	return s_storage->default_framebuffer_;
}

TextureHandle MainRenderer::get_framebuffer_texture(FramebufferHandle handle, uint32_t index)
{
	W_ASSERT(is_valid(handle), "Invalid FramebufferHandle.");
	W_ASSERT(index < s_storage->framebuffer_textures_[handle.index].handles.size(), "Invalid framebuffer texture index.");
	return s_storage->framebuffer_textures_[handle.index].handles[index];
}

/*
		   _____                                          _     
		  / ____|                                        | |    
		 | |     ___  _ __ ___  _ __ ___   __ _ _ __   __| |___ 
		 | |    / _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` / __|
		 | |___| (_) | | | | | | | | | | | (_| | | | | (_| \__ \
		  \_____\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_|___/
*/

// Helpers
static void sort_commands()
{
	// Keys stored separately from commands to avoid touching data too
	// much during sort calls
    std::sort(std::begin(s_storage->pre_buffer_.entries), std::begin(s_storage->pre_buffer_.entries) + s_storage->pre_buffer_.count, 
        [&](const CommandBuffer::Entry& item1, const CommandBuffer::Entry& item2)
        {
        	return item1.first > item2.first;
        });
    std::sort(std::begin(s_storage->post_buffer_.entries), std::begin(s_storage->post_buffer_.entries) + s_storage->post_buffer_.count, 
        [&](const CommandBuffer::Entry& item1, const CommandBuffer::Entry& item2)
        {
        	return item1.first > item2.first;
        });
}

inline CommandBuffer& get_command_buffer(MainRenderer::Phase phase)
{
	switch(phase)
	{
		case MainRenderer::Phase::Pre:  return s_storage->pre_buffer_;
		case MainRenderer::Phase::Post: return s_storage->post_buffer_;
	}
}

inline CommandBuffer& get_command_buffer(RenderCommand command)
{
	MainRenderer::Phase phase = (command < RenderCommand::Post) ? MainRenderer::Phase::Pre : MainRenderer::Phase::Post;
	return get_command_buffer(phase);
}

inline void push_command(RenderCommand type, void* cmd)
{
	auto& cmdbuf = get_command_buffer(type);
	uint64_t key = ~uint64_t(cmdbuf.count);
	cmdbuf.entries[cmdbuf.count++] = {key, cmd};
}

IndexBufferHandle MainRenderer::create_index_buffer(uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode)
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
		auxiliary = W_NEW_ARRAY_DYNAMIC(uint32_t, count, s_storage->auxiliary_arena_);
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

VertexBufferLayoutHandle MainRenderer::create_vertex_buffer_layout(const std::initializer_list<BufferLayoutElement>& elements)
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
	BufferLayoutElement* auxiliary = W_NEW_ARRAY_DYNAMIC(BufferLayoutElement, elts.size(), s_storage->auxiliary_arena_);
	memcpy(auxiliary, elts.data(), elts.size() * sizeof(BufferLayoutElement));
	cmdbuf.write(&auxiliary);

	push_command(type, cmd);
	return handle;
}

VertexBufferHandle MainRenderer::create_vertex_buffer(VertexBufferLayoutHandle layout, float* vertex_data, uint32_t count, DrawMode mode)
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
		auxiliary = W_NEW_ARRAY_DYNAMIC(float, count, s_storage->auxiliary_arena_);
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

VertexArrayHandle MainRenderer::create_vertex_array(VertexBufferHandle vb, IndexBufferHandle ib)
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

UniformBufferHandle MainRenderer::create_uniform_buffer(const std::string& name, void* data, uint32_t size, DrawMode mode)
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
		auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage->auxiliary_arena_);
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

ShaderStorageBufferHandle MainRenderer::create_shader_storage_buffer(const std::string& name, void* data, uint32_t size, DrawMode mode)
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
		auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage->auxiliary_arena_);
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

ShaderHandle MainRenderer::create_shader(const fs::path& filepath, const std::string& name)
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

TextureHandle MainRenderer::create_texture_2D(const Texture2DDescriptor& desc)
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

FramebufferHandle MainRenderer::create_framebuffer(uint32_t width, uint32_t height, bool depth, bool stencil, const FramebufferLayout& layout)
{
	RenderCommand type = RenderCommand::CreateFramebuffer;
	auto& cmdbuf = get_command_buffer(type).storage;

	void* cmd = cmdbuf.head();
	FramebufferHandle handle = { s_storage->handles_[FramebufferHandleT]->acquire() };
	W_ASSERT(is_valid(handle), "No more free handle in handle pool.");

	// Create handles for framebuffer textures
	FramebufferTextureVector texture_vector;
	uint32_t tex_count = depth ? layout.get_count()+1 : layout.get_count(); // Take the depth texture into account
	for(uint32_t ii=0; ii<tex_count; ++ii)
	{
		TextureHandle tex_handle = { s_storage->handles_[TextureHandleT]->acquire() };
		texture_vector.handles.push_back(tex_handle);
	}
	s_storage->framebuffer_textures_.insert(std::make_pair(handle.index, texture_vector));

	uint32_t count = layout.get_count();
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&width);
	cmdbuf.write(&height);
	cmdbuf.write(&depth);
	cmdbuf.write(&stencil);
	cmdbuf.write(&count);

	// Write auxiliary data
	FramebufferLayoutElement* auxiliary = W_NEW_ARRAY_DYNAMIC(FramebufferLayoutElement, layout.get_count(), s_storage->auxiliary_arena_);
	memcpy(auxiliary, layout.data(), layout.get_count() * sizeof(FramebufferLayoutElement));
	cmdbuf.write(&auxiliary);

	push_command(type, cmd);
	return handle;
}

void MainRenderer::update_index_buffer(IndexBufferHandle handle, uint32_t* data, uint32_t count)
{
	W_ASSERT(is_valid(handle), "Invalid IndexBufferHandle!");
	W_ASSERT(data, "No data!");

	RenderCommand type = RenderCommand::UpdateIndexBuffer;
	auto& cmdbuf = get_command_buffer(type).storage;

	void* cmd = cmdbuf.head();
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&count);
	uint32_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint32_t, count, s_storage->auxiliary_arena_);
	memcpy(auxiliary, data, count);
	cmdbuf.write(&auxiliary);

	push_command(type, cmd);
}

void MainRenderer::update_vertex_buffer(VertexBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(is_valid(handle), "Invalid VertexBufferHandle!");
	W_ASSERT(data, "No data!");

	RenderCommand type = RenderCommand::UpdateVertexBuffer;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&size);
	uint8_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage->auxiliary_arena_);
	memcpy(auxiliary, data, size);
	cmdbuf.write(&auxiliary);

	push_command(type, cmd);
}

void MainRenderer::update_uniform_buffer(UniformBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(is_valid(handle), "Invalid UniformBufferHandle!");
	W_ASSERT(data, "No data!");

	RenderCommand type = RenderCommand::UpdateUniformBuffer;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&size);
	uint8_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage->auxiliary_arena_);
	memcpy(auxiliary, data, size);
	cmdbuf.write(&auxiliary);

	push_command(type, cmd);
}

void MainRenderer::update_shader_storage_buffer(ShaderStorageBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(is_valid(handle), "Invalid ShaderStorageBufferHandle!");
	W_ASSERT(data, "No data!");

	RenderCommand type = RenderCommand::UpdateShaderStorageBuffer;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&handle);
	cmdbuf.write(&size);
	uint8_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage->auxiliary_arena_);
	memcpy(auxiliary, data, size);
	cmdbuf.write(&auxiliary);

	push_command(type, cmd);
}

void MainRenderer::shader_attach_uniform_buffer(ShaderHandle shader, UniformBufferHandle ubo)
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

void MainRenderer::shader_attach_storage_buffer(ShaderHandle shader, ShaderStorageBufferHandle ssbo)
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

void MainRenderer::update_framebuffer(FramebufferHandle fb, uint32_t width, uint32_t height)
{
	W_ASSERT(is_valid(fb), "Invalid FramebufferHandle!");

	RenderCommand type = RenderCommand::UpdateFramebuffer;
	auto& cmdbuf = get_command_buffer(type).storage;
	void* cmd = cmdbuf.head();
	
	cmdbuf.write(&type);
	cmdbuf.write(&fb);
	cmdbuf.write(&width);
	cmdbuf.write(&height);

	push_command(type, cmd);
}

void MainRenderer::destroy(IndexBufferHandle handle)
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

void MainRenderer::destroy(VertexBufferLayoutHandle handle)
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

void MainRenderer::destroy(VertexBufferHandle handle)
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

void MainRenderer::destroy(VertexArrayHandle handle)
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

void MainRenderer::destroy(UniformBufferHandle handle)
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

void MainRenderer::destroy(ShaderStorageBufferHandle handle)
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

void MainRenderer::destroy(ShaderHandle handle)
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

void MainRenderer::destroy(TextureHandle handle)
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

void MainRenderer::destroy(FramebufferHandle handle)
{
	W_ASSERT(is_valid(handle), "Invalid FramebufferHandle!");
	s_storage->handles_[FramebufferHandleT]->release(handle.index);

	RenderCommand type = RenderCommand::DestroyFramebuffer;
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

void create_framebuffer(memory::LinearBuffer<>& buf)
{
	uint32_t width;
	uint32_t height;
	uint32_t count;
	bool depth;
	bool stencil;
	FramebufferHandle handle;
	FramebufferLayoutElement* auxiliary;
	buf.read(&handle);
	buf.read(&width);
	buf.read(&height);
	buf.read(&depth);
	buf.read(&stencil);
	buf.read(&count);
	buf.read(&auxiliary);

	FramebufferLayout layout(auxiliary, count);
	s_storage->framebuffers[handle.index] = Framebuffer::create(width, height, layout, depth, stencil);

	// Register framebuffer textures as regular textures accessible by handles
	auto& fb = s_storage->framebuffers[handle.index];
	const auto& texture_vector = s_storage->framebuffer_textures_[handle.index];
	for(uint32_t ii=0; ii<texture_vector.handles.size(); ++ii)
		s_storage->textures[texture_vector.handles[ii].index] = fb->get_shared_texture(ii);
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
	shader.attach_uniform_buffer(s_storage->uniform_buffers[ubo_handle.index]);
}

void shader_attach_storage_buffer(memory::LinearBuffer<>& buf)
{
	ShaderHandle shader_handle;
	ShaderStorageBufferHandle ssbo_handle;
	buf.read(&shader_handle);
	buf.read(&ssbo_handle);

	auto& shader = *s_storage->shaders[shader_handle.index];
	shader.attach_shader_storage(s_storage->shader_storage_buffers[ssbo_handle.index]);
}

void update_framebuffer(memory::LinearBuffer<>& buf)
{
	FramebufferHandle fb_handle;
	uint32_t width;
	uint32_t height;

	buf.read(&fb_handle);
	buf.read(&width);
	buf.read(&height);

	bool has_depth   = s_storage->framebuffers[fb_handle.index]->has_depth();
	bool has_stencil = s_storage->framebuffers[fb_handle.index]->has_stencil();
	auto layout      = s_storage->framebuffers[fb_handle.index]->get_layout();

	s_storage->framebuffers[fb_handle.index] = Framebuffer::create(width, height, layout, has_depth, has_stencil);

	// Update framebuffer textures
	auto& fb = s_storage->framebuffers[fb_handle.index];
	auto& texture_vector = s_storage->framebuffer_textures_[fb_handle.index];
	for(uint32_t ii=0; ii<texture_vector.handles.size(); ++ii)
		s_storage->textures[texture_vector.handles[ii].index] = fb->get_shared_texture(ii);
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

void destroy_framebuffer(memory::LinearBuffer<>& buf)
{
	FramebufferHandle handle;
	buf.read(&handle);
	s_storage->framebuffers[handle.index] = nullptr;

	// Delete framebuffer textures
	auto& texture_vector = s_storage->framebuffer_textures_[handle.index];
	for(uint32_t ii=0; ii<texture_vector.handles.size(); ++ii)
	{
		uint16_t tex_index = texture_vector.handles[ii].index;
		s_storage->textures[tex_index] = nullptr;
		s_storage->handles_[TextureHandleT]->release(tex_index);
	}
	s_storage->framebuffer_textures_.erase(handle.index);
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
	&dispatch::create_shader,
	&dispatch::create_texture_2D,
	&dispatch::create_framebuffer,
	&dispatch::update_index_buffer,
	&dispatch::update_vertex_buffer,
	&dispatch::update_uniform_buffer,
	&dispatch::update_shader_storage_buffer,
	&dispatch::shader_attach_uniform_buffer,
	&dispatch::shader_attach_storage_buffer,
	&dispatch::update_framebuffer,

	&dispatch::nop,

	&dispatch::destroy_index_buffer,
	&dispatch::destroy_vertex_buffer_layout,
	&dispatch::destroy_vertex_buffer,
	&dispatch::destroy_vertex_array,
	&dispatch::destroy_uniform_buffer,
	&dispatch::destroy_shader_storage_buffer,
	&dispatch::destroy_shader,
	&dispatch::destroy_texture_2D,
	&dispatch::destroy_framebuffer,
};

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

RenderQueue::RenderQueue(SortKey::Order order, memory::HeapArea& area):
order_(order),
command_buffer_(area.require_block(cfg::get<size_t>("erwin.renderer.memory.queue_buffer"_h, 512_kB)))
{
	render_target_.index = 0;
}

RenderQueue::~RenderQueue()
{

}

void RenderQueue::set_clear_color(uint8_t R, uint8_t G, uint8_t B, uint8_t A)
{
	clear_color_ = (R << 0) + (G << 8) + (B << 16) + (A << 24);
	Gfx::device->set_clear_color(R/255.f, G/255.f, B/255.f, A/255.f);
}

void RenderQueue::set_render_target(FramebufferHandle fb)
{
	render_target_ = fb;
}

void RenderQueue::sort()
{
	// Keys stored separately from commands to avoid touching data too
	// much during sort calls
    std::sort(std::begin(command_buffer_.entries), std::begin(command_buffer_.entries) + command_buffer_.count, 
        [&](const CommandBuffer::Entry& item1, const CommandBuffer::Entry& item2)
        {
        	return item1.first > item2.first;
        });
}

void RenderQueue::reset()
{
	command_buffer_.reset();
}

void RenderQueue::submit(const DrawCall& dc)
{
	W_ASSERT(is_valid(dc.VAO), "Invalid VertexArrayHandle!");
	W_ASSERT(is_valid(dc.shader), "Invalid ShaderHandle!");

	auto& cmdbuf = command_buffer_.storage;
	void* cmd = cmdbuf.head();

	cmdbuf.write(&dc.type);
	cmdbuf.write(&dc.state_flags);
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
	void* ubo_data = nullptr;
	void* ssbo_data = nullptr;
	if(dc.UBO_data)
	{
		ubo_data = W_NEW_ARRAY_DYNAMIC(uint8_t, dc.UBO_size, s_storage->auxiliary_arena_);
		memcpy(ubo_data, dc.UBO_data, dc.UBO_size);
	}
	if(dc.SSBO_data)
	{
		ssbo_data = dc.SSBO_data;
	}
	cmdbuf.write(&ubo_data);
	cmdbuf.write(&ssbo_data);

	command_buffer_.entries[command_buffer_.count++] = {dc.key.encode(order_), cmd};
}

static void render_dispatch(memory::LinearBuffer<>& buf)
{
#pragma pack(push,1)
	struct
	{
		DrawCall::Type type;
		uint64_t state_flags;
		VertexArrayHandle va_handle;
		ShaderHandle shader_handle;
		UniformBufferHandle ubo_handle;
		ShaderStorageBufferHandle ssbo_handle;
		uint32_t ubo_size;
		uint32_t ssbo_size;
		uint32_t count;
		uint32_t instance_count;
		uint32_t offset;
		hash_t sampler;
		TextureHandle texture_handle;
		void* ubo_data;
		void* ssbo_data;
	} dc;
#pragma pack(pop)
	buf.read(&dc); // Read all in one go

	static uint64_t last_state = 0xffffffffffffffff;
	if(dc.state_flags != last_state)
	{
		PassState state;
		state.decode(dc.state_flags);

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
	
		last_state = dc.state_flags;
	}

	auto& va = *s_storage->vertex_arrays[dc.va_handle.index];
	auto& shader = *s_storage->shaders[dc.shader_handle.index];

	shader.bind();

	if(dc.ubo_data)
	{
		auto& ubo = *s_storage->uniform_buffers[dc.ubo_handle.index];
		ubo.stream(dc.ubo_data, dc.ubo_size, 0);
		// shader.bind_uniform_buffer(ubo);
	}
	if(dc.ssbo_data)
	{
		auto& ssbo = *s_storage->shader_storage_buffers[dc.ssbo_handle.index];
		ssbo.stream(dc.ssbo_data, dc.ssbo_size, 0);
		// shader.bind_shader_storage(ssbo);
	}
	if(is_valid(dc.texture_handle))
	{
		auto& texture = *s_storage->textures[dc.texture_handle.index];
		shader.attach_texture(dc.sampler, texture);
		texture.bind();
	}

	switch(dc.type)
	{
		case DrawCall::Indexed:
			Gfx::device->draw_indexed(va, dc.count, dc.offset);
			break;
		case DrawCall::IndexedInstanced:
			Gfx::device->draw_indexed_instanced(va, dc.instance_count);
			break;
		default:
			break;
	}
}

void RenderQueue::flush()
{
	if(command_buffer_.count == 0)
		return;

	if(render_target_ == s_storage->default_framebuffer_)
		Gfx::device->bind_default_framebuffer();
	else
	{
		W_ASSERT(is_valid(render_target_), "Invalid FramebufferHandle!");
		s_storage->framebuffers[render_target_.index]->bind();
	}
	Gfx::device->clear(ClearFlags::CLEAR_COLOR_FLAG | ClearFlags::CLEAR_DEPTH_FLAG); // TMP: flags will change for each queue

	for(int ii=0; ii<command_buffer_.count; ++ii)
	{
		auto&& [key,cmd] = command_buffer_.entries[ii];
		command_buffer_.storage.seek(cmd);
		render_dispatch(command_buffer_.storage);
	}
}

static void flush_command_buffer(CommandBuffer& cmdbuf)
{
	for(int ii=0; ii<cmdbuf.count; ++ii)
	{
		auto&& [key,cmd] = cmdbuf.entries[ii];
		cmdbuf.storage.seek(cmd);
		uint16_t type;
		cmdbuf.storage.read(&type);
		(*backend_dispatch[type])(cmdbuf.storage);
	}
	cmdbuf.reset();
}

void MainRenderer::flush()
{
	if(s_storage->profiling_enabled)
		s_storage->query_timer->start();

	// Sort command buffers
	sort_commands();
	// Dispatch pre buffer commands
	flush_command_buffer(s_storage->pre_buffer_);
	// Sort and flush each queue
	for(auto&& [name, queue]: s_storage->queues_)
	{
		queue.sort();
		queue.flush();
		queue.reset();
	}
	// Dispatch post buffer commands
	flush_command_buffer(s_storage->post_buffer_);
	// Reset auxiliary memory arena for next frame
	s_storage->auxiliary_arena_.get_allocator().reset();

	if(s_storage->profiling_enabled)
	{
		auto render_duration = s_storage->query_timer->stop();
		s_storage->stats.render_time = std::chrono::duration_cast<std::chrono::microseconds>(render_duration).count();
	}
}

} // namespace erwin