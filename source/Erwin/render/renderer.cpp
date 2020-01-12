#include "render/renderer.h"

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
#include "filesystem/filesystem.h"

namespace erwin
{

// This macro allows us to perform the same action for all handle structs.
// When creating a new renderer handle type, simply add a line here, and change allocation size.
#define FOR_ALL_HANDLES                        \
		DO_ACTION( IndexBufferHandle )         \
		DO_ACTION( VertexBufferLayoutHandle )  \
		DO_ACTION( VertexBufferHandle )        \
		DO_ACTION( VertexArrayHandle )         \
		DO_ACTION( UniformBufferHandle )       \
		DO_ACTION( ShaderStorageBufferHandle ) \
		DO_ACTION( TextureHandle )             \
		DO_ACTION( ShaderHandle )              \
		DO_ACTION( FramebufferHandle )

constexpr std::size_t k_handle_alloc_size = 9 * 2 * sizeof(HandlePoolT<k_max_render_handles>);

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
constexpr uint8_t  k_view_bits       = 16;
constexpr uint8_t  k_draw_type_bits  = 2;
constexpr uint8_t  k_shader_bits     = 8;
constexpr uint8_t  k_depth_bits      = 32;
constexpr uint8_t  k_seq_bits        = 32;
constexpr uint64_t k_view_shift      = uint8_t(64)        - k_view_bits;
constexpr uint64_t k_draw_type_shift = k_view_shift       - k_draw_type_bits;
constexpr uint64_t k_1_shader_shift  = k_draw_type_shift  - k_shader_bits;
constexpr uint64_t k_1_depth_shift   = k_1_shader_shift   - k_depth_bits;
constexpr uint64_t k_2_depth_shift   = k_draw_type_shift  - k_depth_bits;
constexpr uint64_t k_2_shader_shift  = k_2_depth_shift    - k_shader_bits;
constexpr uint64_t k_3_seq_shift     = k_draw_type_shift  - k_seq_bits;
constexpr uint64_t k_3_shader_shift  = k_3_seq_shift      - k_shader_bits;
constexpr uint64_t k_view_mask       = uint64_t(0x0000ffff) << k_view_shift;
constexpr uint64_t k_draw_type_mask  = uint64_t(0x00000003) << k_draw_type_shift;
constexpr uint64_t k_1_shader_mask   = uint64_t(0x000000ff) << k_1_shader_shift;
constexpr uint64_t k_1_depth_mask    = uint64_t(0xffffffff) << k_1_depth_shift;
constexpr uint64_t k_2_depth_mask    = uint64_t(0xffffffff) << k_2_depth_shift;
constexpr uint64_t k_2_shader_mask   = uint64_t(0x000000ff) << k_2_shader_shift;
constexpr uint64_t k_3_seq_mask      = uint64_t(0xffffffff) << k_3_seq_shift;
constexpr uint64_t k_3_shader_mask   = uint64_t(0x000000ff) << k_3_shader_shift;

uint64_t SortKey::encode() const
{
	uint64_t head = ((uint64_t(view)      << k_view_shift     ) & k_view_mask)
				  | ((uint64_t(blending)  << k_draw_type_shift) & k_draw_type_mask);

	uint64_t body = 0;
	switch(order)
	{
		case SortKey::Order::ByShader:
		{
			body |= ((uint64_t(shader)    << k_1_shader_shift) & k_1_shader_mask)
				 |  ((uint64_t(depth)     << k_1_depth_shift)  & k_1_depth_mask);
			break;
		}
		case SortKey::Order::ByDepthDescending:
		{
			body |= ((uint64_t(depth)    << k_2_depth_shift)  & k_2_depth_mask)
				 |  ((uint64_t(shader)    << k_2_shader_shift) & k_2_shader_mask);
			break;
		}
		case SortKey::Order::ByDepthAscending:
		{
			body |= ((uint64_t(~depth)     << k_2_depth_shift)  & k_2_depth_mask)
				 |  ((uint64_t(shader)    << k_2_shader_shift) & k_2_shader_mask);
			break;
		}
		case SortKey::Order::Sequential:
		{
			body |= ((uint64_t(sequence) << k_3_seq_shift)    & k_3_seq_mask)
				 |  ((uint64_t(shader)    << k_3_shader_shift) & k_3_shader_mask);
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

static struct RendererStorage
{
	RendererStorage() = default;
	~RendererStorage() = default;

	inline void clear_resources()
	{
		std::fill(std::begin(index_buffers),          std::end(index_buffers),          nullptr);
		std::fill(std::begin(vertex_buffer_layouts),  std::end(vertex_buffer_layouts),  nullptr);
		std::fill(std::begin(vertex_buffers),         std::end(vertex_buffers),         nullptr);
		std::fill(std::begin(vertex_arrays),          std::end(vertex_arrays),          nullptr);
		std::fill(std::begin(uniform_buffers),        std::end(uniform_buffers),        nullptr);
		std::fill(std::begin(shader_storage_buffers), std::end(shader_storage_buffers), nullptr);
		std::fill(std::begin(textures),               std::end(textures),               nullptr);
		std::fill(std::begin(shaders),                std::end(shaders),                nullptr);
		std::fill(std::begin(framebuffers),           std::end(framebuffers),           nullptr);
	}

	inline void init(memory::HeapArea* area)
	{
		renderer_memory_ = area;
		pre_buffer_.init(*renderer_memory_, cfg::get<size_t>("erwin.memory.renderer.pre_buffer"_h, 512_kB), "CB-Pre");
		post_buffer_.init(*renderer_memory_, cfg::get<size_t>("erwin.memory.renderer.post_buffer"_h, 512_kB), "CB-Post");
		auxiliary_arena_.init(*renderer_memory_, cfg::get<size_t>("erwin.memory.renderer.auxiliary_arena"_h, 2_MB), "Auxiliary");
		handle_arena_.init(*renderer_memory_, k_handle_alloc_size, "RenderHandles");
		queue_.init(*renderer_memory_);

		clear_resources();

		// Init handle pools
		#define DO_ACTION( HANDLE_NAME ) HANDLE_NAME::init_pool(handle_arena_);
		FOR_ALL_HANDLES
		#undef DO_ACTION

		// Init render queue
		queue_.set_clear_color(glm::vec4(0.f,0.f,0.f,0.f));
	}

	inline void release()
	{
		clear_resources();
		default_framebuffer_.release();

		// Destroy handle pools
		#define DO_ACTION( HANDLE_NAME ) HANDLE_NAME::destroy_pool(handle_arena_);
		FOR_ALL_HANDLES
		#undef DO_ACTION
	}

	// TODO: Drop WRefs and use arenas (with pool allocator?) to allocate memory for these objects
	WRef<IndexBuffer>         index_buffers[k_max_render_handles];
	WRef<BufferLayout>        vertex_buffer_layouts[k_max_render_handles];
	WRef<VertexBuffer>        vertex_buffers[k_max_render_handles];
	WRef<VertexArray>         vertex_arrays[k_max_render_handles];
	WRef<UniformBuffer>       uniform_buffers[k_max_render_handles];
	WRef<ShaderStorageBuffer> shader_storage_buffers[k_max_render_handles];
	WRef<Texture2D>			  textures[k_max_render_handles];
	WRef<Shader>			  shaders[k_max_render_handles];
	WRef<Framebuffer>		  framebuffers[k_max_render_handles];

	FramebufferHandle default_framebuffer_;
	std::map<uint16_t, FramebufferTextureVector> framebuffer_textures_;
	std::map<hash_t, ShaderHandle> shader_names_;
	std::vector<std::function<void(void)>> end_frame_callbacks_;

	WScope<QueryTimer> query_timer;
	bool profiling_enabled;
	RendererStats stats;

	memory::HeapArea* renderer_memory_;
	CommandBuffer pre_buffer_;
	CommandBuffer post_buffer_;
	Renderer::AuxArena auxiliary_arena_;
	LinearArena handle_arena_;
	RenderQueue queue_;
} s_storage;

void Renderer::init(memory::HeapArea& area)
{
    W_PROFILE_RENDER_FUNCTION()
	DLOGN("render") << "[Renderer] Allocating renderer storage." << std::endl;
	
	// Create and initialize storage object
	s_storage.init(&area);
	s_storage.query_timer = QueryTimer::create();
	s_storage.profiling_enabled = false;
	s_storage.default_framebuffer_ = FramebufferHandle::acquire();

	DLOGI << "done" << std::endl;

	DLOGN("render") << "[Renderer] Creating main render targets." << std::endl;
	// Create main render targets
	{
	    FramebufferLayout layout =
	    {
	        {"albedo"_h, ImageFormat::RGBA16F, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE},
	        {"glow"_h,   ImageFormat::RGBA8, MIN_LINEAR | MAG_LINEAR, TextureWrap::CLAMP_TO_EDGE}, // For bloom effect
	    };
	    FramebufferPool::create_framebuffer("LBuffer"_h, make_scope<FbRatioConstraint>(), layout, true);
	}

	DLOGI << "done" << std::endl;
}

void Renderer::shutdown()
{
    W_PROFILE_RENDER_FUNCTION()
	flush();
	DLOGN("render") << "[Renderer] Releasing renderer storage." << std::endl;
	s_storage.release();
	DLOGI << "done" << std::endl;
}

RenderQueue& Renderer::get_queue()
{
	return s_storage.queue_;
}

Renderer::AuxArena& Renderer::get_arena()
{
	return s_storage.auxiliary_arena_;
}

void Renderer::set_end_frame_callback(std::function<void(void)> callback)
{
	s_storage.end_frame_callbacks_.push_back(callback);
}

#ifdef W_DEBUG
void Renderer::set_profiling_enabled(bool value)
{
	s_storage.profiling_enabled = value;
}

const RendererStats& Renderer::get_stats()
{
	return s_storage.stats;
}
#endif

FramebufferHandle Renderer::default_render_target()
{
	return s_storage.default_framebuffer_;
}

TextureHandle Renderer::get_framebuffer_texture(FramebufferHandle handle, uint32_t index)
{
	W_ASSERT(handle.is_valid(), "Invalid FramebufferHandle.");
	W_ASSERT(index < s_storage.framebuffer_textures_[handle.index].handles.size(), "Invalid framebuffer texture index.");
	return s_storage.framebuffer_textures_[handle.index].handles[index];
}

uint32_t Renderer::get_framebuffer_texture_count(FramebufferHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid FramebufferHandle.");
	return s_storage.framebuffer_textures_[handle.index].handles.size();
}

#ifdef W_DEBUG
void* Renderer::get_native_texture_handle(TextureHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid TextureHandle.");
	return s_storage.textures[handle.index]->get_native_handle();
}
#endif

VertexBufferLayoutHandle Renderer::create_vertex_buffer_layout(const std::vector<BufferLayoutElement>& elements)
{
	VertexBufferLayoutHandle handle = VertexBufferLayoutHandle::acquire();
	W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

	s_storage.vertex_buffer_layouts[handle.index] = make_ref<BufferLayout>((BufferLayoutElement*)elements.data(), elements.size());

	return handle;
}

const BufferLayout& Renderer::get_vertex_buffer_layout(VertexBufferLayoutHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid VertexBufferLayoutHandle!");

	return *s_storage.vertex_buffer_layouts[handle.index];
}

/*
		   _____                                          _     
		  / ____|                                        | |    
		 | |     ___  _ __ ___  _ __ ___   __ _ _ __   __| |___ 
		 | |    / _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` / __|
		 | |___| (_) | | | | | | | | | | | (_| | | | | (_| \__ \
		  \_____\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_|___/
*/

enum class RenderCommand: uint16_t
{
	CreateIndexBuffer,
	CreateVertexBuffer,
	CreateVertexArray,
	CreateUniformBuffer,
	CreateShaderStorageBuffer,
	CreateShader,
	CreateTexture2D,
	CreateFramebuffer,

	UpdateIndexBuffer,
	UpdateVertexBuffer,
	UpdateUniformBuffer,
	UpdateShaderStorageBuffer,
	ShaderAttachUniformBuffer,
	ShaderAttachStorageBuffer,
	UpdateFramebuffer,
	ClearFramebuffers,

	Post,

	FramebufferScreenshot,

	DestroyIndexBuffer,
	DestroyVertexBufferLayout,
	DestroyVertexBuffer,
	DestroyVertexArray,
	DestroyUniformBuffer,
	DestroyShaderStorageBuffer,
	DestroyShader,
	DestroyTexture2D,
	DestroyFramebuffer,

	Count
};

// Helper class for command buffer access
class CommandWriter
{
public:
	CommandWriter(RenderCommand type):
	type_(type),
	cmdbuf_(get_command_buffer(type_)),
	head_(cmdbuf_.storage.head())
	{
		cmdbuf_.storage.write(&type_);
	}

	template <typename T>
	inline void write(T* source)                  { cmdbuf_.storage.write(source); }
	inline void write_str(const std::string& str) { cmdbuf_.storage.write_str(str); }

	inline void submit()
	{
		uint64_t key = uint64_t(cmdbuf_.count);
		cmdbuf_.entries[cmdbuf_.count++] = {key, head_};
	}

private:
	enum class Phase
	{
		Pre,
		Post
	};

	inline CommandBuffer& get_command_buffer(Phase phase)
	{
		switch(phase)
		{
			case Phase::Pre:  return s_storage.pre_buffer_;
			case Phase::Post: return s_storage.post_buffer_;
		}
	}

	inline CommandBuffer& get_command_buffer(RenderCommand command)
	{
		Phase phase = (command < RenderCommand::Post) ? Phase::Pre : Phase::Post;
		return get_command_buffer(phase);
	}

private:
	RenderCommand type_;
	CommandBuffer& cmdbuf_;
	void* head_;
};

IndexBufferHandle Renderer::create_index_buffer(const uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode)
{
	IndexBufferHandle handle = IndexBufferHandle::acquire();
	W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

	// Allocate auxiliary data
	uint32_t* auxiliary = nullptr;
	if(index_data)
	{
		auxiliary = W_NEW_ARRAY_DYNAMIC(uint32_t, count, s_storage.auxiliary_arena_);
		memcpy(auxiliary, index_data, count * sizeof(uint32_t));
	}
	else
		W_ASSERT(mode != DrawMode::Static, "Index data can't be null in static mode.");

	// Write data
	CommandWriter cw(RenderCommand::CreateIndexBuffer);
	cw.write(&handle);
	cw.write(&count);
	cw.write(&primitive);
	cw.write(&mode);
	cw.write(&auxiliary);
	cw.submit();

	return handle;
}

VertexBufferHandle Renderer::create_vertex_buffer(VertexBufferLayoutHandle layout, const float* vertex_data, uint32_t count, DrawMode mode)
{
	W_ASSERT(layout.is_valid(), "Invalid VertexBufferLayoutHandle!");

	VertexBufferHandle handle = VertexBufferHandle::acquire();
	W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

	// Allocate auxiliary data
	float* auxiliary = nullptr;
	if(vertex_data)
	{
		auxiliary = W_NEW_ARRAY_DYNAMIC(float, count, s_storage.auxiliary_arena_);
		memcpy(auxiliary, vertex_data, count * sizeof(float));
	}
	else
		W_ASSERT(mode != DrawMode::Static, "Vertex data can't be null in static mode.");

	// Write data
	CommandWriter cw(RenderCommand::CreateVertexBuffer);
	cw.write(&handle);
	cw.write(&layout);
	cw.write(&count);
	cw.write(&mode);
	cw.write(&auxiliary);
	cw.submit();

	return handle;
}

VertexArrayHandle Renderer::create_vertex_array(VertexBufferHandle vb, IndexBufferHandle ib)
{
	W_ASSERT(vb.is_valid(), "Invalid VertexBufferHandle!");
	W_ASSERT(ib.is_valid(), "Invalid IndexBufferHandle!");

	VertexArrayHandle handle = VertexArrayHandle::acquire();
	W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

	// Write data
	CommandWriter cw(RenderCommand::CreateVertexArray);
	cw.write(&handle);
	cw.write(&vb);
	cw.write(&ib);
	cw.submit();

	return handle;
}

UniformBufferHandle Renderer::create_uniform_buffer(const std::string& name, void* data, uint32_t size, DrawMode mode)
{
	UniformBufferHandle handle = UniformBufferHandle::acquire();
	W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

	// Allocate auxiliary data
	uint8_t* auxiliary = nullptr;
	if(data)
	{
		auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage.auxiliary_arena_);
		memcpy(auxiliary, data, size);
	}
	else
		W_ASSERT(mode != DrawMode::Static, "UBO data can't be null in static mode.");

	// Write data
	CommandWriter cw(RenderCommand::CreateUniformBuffer);
	cw.write(&handle);
	cw.write(&size);
	cw.write(&mode);
	cw.write_str(name);
	cw.write(&auxiliary);
	cw.submit();

	return handle;
}

ShaderStorageBufferHandle Renderer::create_shader_storage_buffer(const std::string& name, void* data, uint32_t size, DrawMode mode)
{
	ShaderStorageBufferHandle handle = ShaderStorageBufferHandle::acquire();
	W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

	// Allocate auxiliary data
	uint8_t* auxiliary = nullptr;
	if(data)
	{
		auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage.auxiliary_arena_);
		memcpy(auxiliary, data, size);
	}
	else
		W_ASSERT(mode != DrawMode::Static, "SSBO data can't be null in static mode.");

	// Write data
	CommandWriter cw(RenderCommand::CreateShaderStorageBuffer);
	cw.write(&handle);
	cw.write(&size);
	cw.write(&mode);
	cw.write_str(name);
	cw.write(&auxiliary);
	cw.submit();

	return handle;
}

ShaderHandle Renderer::create_shader(const fs::path& filepath, const std::string& name)
{
	ShaderHandle handle = ShaderHandle::acquire();
	W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

	CommandWriter cw(RenderCommand::CreateShader);
	cw.write(&handle);
	cw.write_str(filepath.string());
	cw.write_str(name);
	cw.submit();

	return handle;
}

TextureHandle Renderer::create_texture_2D(const Texture2DDescriptor& desc)
{
	TextureHandle handle = TextureHandle::acquire();
	W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

	CommandWriter cw(RenderCommand::CreateTexture2D);
	cw.write(&handle);
	cw.write(&desc);
	cw.submit();

	return handle;
}

FramebufferHandle Renderer::create_framebuffer(uint32_t width, uint32_t height, bool depth, bool stencil, const FramebufferLayout& layout)
{
	FramebufferHandle handle = FramebufferHandle::acquire();
	W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

	// Create handles for framebuffer textures
	FramebufferTextureVector texture_vector;
	uint32_t tex_count = depth ? layout.get_count()+1 : layout.get_count(); // Take the depth texture into account
	for(uint32_t ii=0; ii<tex_count; ++ii)
	{
		TextureHandle tex_handle = TextureHandle::acquire();
		W_ASSERT(tex_handle.is_valid(), "No more free handle in handle pool.");
		texture_vector.handles.push_back(tex_handle);
	}
	s_storage.framebuffer_textures_.insert(std::make_pair(handle.index, texture_vector));
	uint32_t count = layout.get_count();

	// Allocate auxiliary data
	FramebufferLayoutElement* auxiliary = W_NEW_ARRAY_DYNAMIC(FramebufferLayoutElement, count, s_storage.auxiliary_arena_);
	memcpy(auxiliary, layout.data(), count * sizeof(FramebufferLayoutElement));

	CommandWriter cw(RenderCommand::CreateFramebuffer);
	cw.write(&handle);
	cw.write(&width);
	cw.write(&height);
	cw.write(&depth);
	cw.write(&stencil);
	cw.write(&count);
	cw.write(&auxiliary);
	cw.submit();

	return handle;
}

void Renderer::update_index_buffer(IndexBufferHandle handle, uint32_t* data, uint32_t count)
{
	W_ASSERT(handle.is_valid(), "Invalid IndexBufferHandle!");
	W_ASSERT(data, "No data!");

	uint32_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint32_t, count, s_storage.auxiliary_arena_);
	memcpy(auxiliary, data, count);

	CommandWriter cw(RenderCommand::UpdateIndexBuffer);
	cw.write(&handle);
	cw.write(&count);
	cw.write(&auxiliary);
	cw.submit();
}

void Renderer::update_vertex_buffer(VertexBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(handle.is_valid(), "Invalid VertexBufferHandle!");
	W_ASSERT(data, "No data!");

	uint8_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage.auxiliary_arena_);
	memcpy(auxiliary, data, size);

	CommandWriter cw(RenderCommand::UpdateVertexBuffer);
	cw.write(&handle);
	cw.write(&size);
	cw.write(&auxiliary);
	cw.submit();
}

void Renderer::update_uniform_buffer(UniformBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(handle.is_valid(), "Invalid UniformBufferHandle!");
	W_ASSERT(data, "No data!");

	uint8_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage.auxiliary_arena_);
	memcpy(auxiliary, data, size);

	CommandWriter cw(RenderCommand::UpdateUniformBuffer);
	cw.write(&handle);
	cw.write(&size);
	cw.write(&auxiliary);
	cw.submit();
}

void Renderer::update_shader_storage_buffer(ShaderStorageBufferHandle handle, void* data, uint32_t size)
{
	W_ASSERT(handle.is_valid(), "Invalid ShaderStorageBufferHandle!");
	W_ASSERT(data, "No data!");

	uint8_t* auxiliary = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage.auxiliary_arena_);
	memcpy(auxiliary, data, size);

	CommandWriter cw(RenderCommand::UpdateShaderStorageBuffer);
	cw.write(&handle);
	cw.write(&size);
	cw.write(&auxiliary);
	cw.submit();
}

void Renderer::shader_attach_uniform_buffer(ShaderHandle shader, UniformBufferHandle ubo)
{
	W_ASSERT(shader.is_valid(), "Invalid ShaderHandle!");
	W_ASSERT(ubo.is_valid(), "Invalid UniformBufferHandle!");

	CommandWriter cw(RenderCommand::ShaderAttachUniformBuffer);
	cw.write(&shader);
	cw.write(&ubo);
	cw.submit();
}

void Renderer::shader_attach_storage_buffer(ShaderHandle shader, ShaderStorageBufferHandle ssbo)
{
	W_ASSERT(shader.is_valid(), "Invalid ShaderHandle!");
	W_ASSERT(ssbo.is_valid(), "Invalid ShaderStorageBufferHandle!");

	CommandWriter cw(RenderCommand::ShaderAttachStorageBuffer);
	cw.write(&shader);
	cw.write(&ssbo);
	cw.submit();
}

void Renderer::update_framebuffer(FramebufferHandle fb, uint32_t width, uint32_t height)
{
	W_ASSERT(fb.is_valid(), "Invalid FramebufferHandle!");

	CommandWriter cw(RenderCommand::UpdateFramebuffer);
	cw.write(&fb);
	cw.write(&width);
	cw.write(&height);
	cw.submit();
}

void Renderer::clear_framebuffers()
{
	CommandWriter cw(RenderCommand::ClearFramebuffers);
	cw.submit();
}

void Renderer::framebuffer_screenshot(FramebufferHandle fb, const fs::path& filepath)
{
	W_ASSERT(fb.is_valid(), "Invalid FramebufferHandle!");

	CommandWriter cw(RenderCommand::FramebufferScreenshot);
	cw.write(&fb);
	cw.write_str(filepath.string());
	cw.submit();
}

void Renderer::destroy(IndexBufferHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid IndexBufferHandle!");
	handle.release();

	CommandWriter cw(RenderCommand::DestroyIndexBuffer);
	cw.write(&handle);
	cw.submit();
}

void Renderer::destroy(VertexBufferLayoutHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid VertexBufferLayoutHandle!");
	handle.release();

	CommandWriter cw(RenderCommand::DestroyVertexBufferLayout);
	cw.write(&handle);
	cw.submit();
}

void Renderer::destroy(VertexBufferHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid VertexBufferHandle!");
	handle.release();

	CommandWriter cw(RenderCommand::DestroyVertexBuffer);
	cw.write(&handle);
	cw.submit();
}

void Renderer::destroy(VertexArrayHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid VertexArrayHandle!");
	handle.release();

	CommandWriter cw(RenderCommand::DestroyVertexArray);
	cw.write(&handle);
	cw.submit();
}

void Renderer::destroy(UniformBufferHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid UniformBufferHandle!");
	handle.release();

	CommandWriter cw(RenderCommand::DestroyUniformBuffer);
	cw.write(&handle);
	cw.submit();
}

void Renderer::destroy(ShaderStorageBufferHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid ShaderStorageBufferHandle!");
	handle.release();

	CommandWriter cw(RenderCommand::DestroyShaderStorageBuffer);
	cw.write(&handle);
	cw.submit();
}

void Renderer::destroy(ShaderHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid ShaderHandle!");
	handle.release();
	
	CommandWriter cw(RenderCommand::DestroyShader);
	cw.write(&handle);
	cw.submit();
}

void Renderer::destroy(TextureHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid TextureHandle!");
	handle.release();
	
	CommandWriter cw(RenderCommand::DestroyTexture2D);
	cw.write(&handle);
	cw.submit();
}

void Renderer::destroy(FramebufferHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid FramebufferHandle!");
	handle.release();

	CommandWriter cw(RenderCommand::DestroyFramebuffer);
	cw.write(&handle);
	cw.submit();
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
    W_PROFILE_RENDER_FUNCTION()

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

	s_storage.index_buffers[handle.index] = IndexBuffer::create(auxiliary, count, primitive, mode);
}

void create_vertex_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

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

	const auto& layout = *s_storage.vertex_buffer_layouts[layout_hnd.index];
	s_storage.vertex_buffers[handle.index] = VertexBuffer::create(auxiliary, count, layout, mode);
}

void create_vertex_array(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	VertexArrayHandle handle;
	VertexBufferHandle vb;
	IndexBufferHandle ib;
	buf.read(&handle);
	buf.read(&vb);
	buf.read(&ib);

	s_storage.vertex_arrays[handle.index] = VertexArray::create();
	s_storage.vertex_arrays[handle.index]->set_vertex_buffer(s_storage.vertex_buffers[vb.index]);
	if(ib.is_valid())
		s_storage.vertex_arrays[handle.index]->set_index_buffer(s_storage.index_buffers[ib.index]);
}

void create_uniform_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

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

	s_storage.uniform_buffers[handle.index] = UniformBuffer::create(name, auxiliary, size, mode);
}

void create_shader_storage_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

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

	s_storage.shader_storage_buffers[handle.index] = ShaderStorageBuffer::create(name, auxiliary, size, mode);
}

void create_shader(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	ShaderHandle handle;
	std::string filepath;
	std::string name;
	buf.read(&handle);
	buf.read_str(filepath);
	buf.read_str(name);

	hash_t hname = H_(name.c_str());
	W_ASSERT(s_storage.shader_names_.find(hname)==s_storage.shader_names_.end(), "Shader already loaded.");

	s_storage.shaders[handle.index] = Shader::create(name, fs::path(filepath));
	s_storage.shader_names_.insert(std::pair(hname, handle));
}

void create_texture_2D(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	TextureHandle handle;
	Texture2DDescriptor descriptor;
	buf.read(&handle);
	buf.read(&descriptor);

	s_storage.textures[handle.index] = Texture2D::create(descriptor);
}

void create_framebuffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

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
	s_storage.framebuffers[handle.index] = Framebuffer::create(width, height, layout, depth, stencil);

	// Register framebuffer textures as regular textures accessible by handles
	auto& fb = s_storage.framebuffers[handle.index];
	const auto& texture_vector = s_storage.framebuffer_textures_[handle.index];
	for(uint32_t ii=0; ii<texture_vector.handles.size(); ++ii)
		s_storage.textures[texture_vector.handles[ii].index] = fb->get_shared_texture(ii);
}

void update_index_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	IndexBufferHandle handle;
	uint32_t count;
	uint32_t* auxiliary;
	buf.read(&handle);
	buf.read(&count);
	buf.read(&auxiliary);

	s_storage.index_buffers[handle.index]->map(auxiliary, count);
}

void update_vertex_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	VertexBufferHandle handle;
	uint32_t size;
	uint8_t* auxiliary;
	buf.read(&handle);
	buf.read(&size);
	buf.read(&auxiliary);

	s_storage.vertex_buffers[handle.index]->map(auxiliary, size);
}

void update_uniform_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	UniformBufferHandle handle;
	uint32_t size;
	uint8_t* auxiliary;
	buf.read(&handle);
	buf.read(&size);
	buf.read(&auxiliary);

	s_storage.uniform_buffers[handle.index]->map(auxiliary);
}

void update_shader_storage_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	ShaderStorageBufferHandle handle;
	uint32_t size;
	uint8_t* auxiliary;
	buf.read(&handle);
	buf.read(&size);
	buf.read(&auxiliary);

	s_storage.shader_storage_buffers[handle.index]->map(auxiliary, size);
}

void shader_attach_uniform_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	ShaderHandle shader_handle;
	UniformBufferHandle ubo_handle;
	buf.read(&shader_handle);
	buf.read(&ubo_handle);

	auto& shader = *s_storage.shaders[shader_handle.index];
	shader.attach_uniform_buffer(s_storage.uniform_buffers[ubo_handle.index]);
}

void shader_attach_storage_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	ShaderHandle shader_handle;
	ShaderStorageBufferHandle ssbo_handle;
	buf.read(&shader_handle);
	buf.read(&ssbo_handle);

	auto& shader = *s_storage.shaders[shader_handle.index];
	shader.attach_shader_storage(s_storage.shader_storage_buffers[ssbo_handle.index]);
}

void update_framebuffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	FramebufferHandle fb_handle;
	uint32_t width;
	uint32_t height;

	buf.read(&fb_handle);
	buf.read(&width);
	buf.read(&height);

	bool has_depth   = s_storage.framebuffers[fb_handle.index]->has_depth();
	bool has_stencil = s_storage.framebuffers[fb_handle.index]->has_stencil();
	auto layout      = s_storage.framebuffers[fb_handle.index]->get_layout();

	s_storage.framebuffers[fb_handle.index] = Framebuffer::create(width, height, layout, has_depth, has_stencil);

	// Update framebuffer textures
	auto& fb = s_storage.framebuffers[fb_handle.index];
	auto& texture_vector = s_storage.framebuffer_textures_[fb_handle.index];
	for(uint32_t ii=0; ii<texture_vector.handles.size(); ++ii)
		s_storage.textures[texture_vector.handles[ii].index] = fb->get_shared_texture(ii);
}

void clear_framebuffers(memory::LinearBuffer<>& buf)
{
	FramebufferPool::traverse_framebuffers([](FramebufferHandle handle)
	{
		s_storage.framebuffers[handle.index]->bind();
		Gfx::device->clear(ClearFlags::CLEAR_COLOR_FLAG | ClearFlags::CLEAR_DEPTH_FLAG);
	});
}

void nop(memory::LinearBuffer<>& buf) { }

void framebuffer_screenshot(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

    FramebufferHandle handle;
	std::string filepath;
	buf.read(&handle);
	buf.read_str(filepath);

	s_storage.framebuffers[handle.index]->screenshot(filepath);
}

void destroy_index_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	IndexBufferHandle handle;
	buf.read(&handle);
	s_storage.index_buffers[handle.index] = nullptr;
}

void destroy_vertex_buffer_layout(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	VertexBufferLayoutHandle handle;
	buf.read(&handle);
	s_storage.vertex_buffer_layouts[handle.index] = nullptr;
}

void destroy_vertex_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	VertexBufferHandle handle;
	buf.read(&handle);
	s_storage.vertex_buffers[handle.index] = nullptr;
}

void destroy_vertex_array(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	VertexArrayHandle handle;
	buf.read(&handle);
	s_storage.vertex_arrays[handle.index] = nullptr;
}

void destroy_uniform_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	UniformBufferHandle handle;
	buf.read(&handle);
	s_storage.uniform_buffers[handle.index] = nullptr;
}

void destroy_shader_storage_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	ShaderStorageBufferHandle handle;
	buf.read(&handle);
	s_storage.shader_storage_buffers[handle.index] = nullptr;
}

void destroy_shader(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	ShaderHandle handle;
	buf.read(&handle);
	hash_t hname = H_(s_storage.shaders[handle.index]->get_name().c_str());
	s_storage.shaders[handle.index] = nullptr;
	s_storage.shader_names_.erase(hname);
}

void destroy_texture_2D(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	TextureHandle handle;
	buf.read(&handle);
	s_storage.textures[handle.index] = nullptr;
}

void destroy_framebuffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	FramebufferHandle handle;
	buf.read(&handle);
	s_storage.framebuffers[handle.index] = nullptr;

	// Delete framebuffer textures
	auto& texture_vector = s_storage.framebuffer_textures_[handle.index];
	for(uint32_t ii=0; ii<texture_vector.handles.size(); ++ii)
	{
		uint16_t tex_index = texture_vector.handles[ii].index;
		s_storage.textures[tex_index] = nullptr;
		texture_vector.handles[ii].release();
	}
	s_storage.framebuffer_textures_.erase(handle.index);
}

} // namespace dispatch

typedef void (* backend_dispatch_func_t)(memory::LinearBuffer<>&);
static backend_dispatch_func_t backend_dispatch[(std::size_t)RenderCommand::Count] =
{
	&dispatch::create_index_buffer,
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
	&dispatch::clear_framebuffers,

	&dispatch::nop,

	&dispatch::framebuffer_screenshot,
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

RenderQueue::RenderQueue(memory::HeapArea& area)
{
	init(area);
}

RenderQueue::~RenderQueue()
{

}

void RenderQueue::init(memory::HeapArea& area)
{
	clear_color_ = {0.f,0.f,0.f,0.f};
	command_buffer_.init(area, cfg::get<size_t>("erwin.memory.renderer.queue_buffer"_h, 512_kB), "RenderQueue");
}

void RenderQueue::sort()
{
    W_PROFILE_RENDER_FUNCTION()

	// Keys stored separately from commands to avoid touching data too
	// much during sort calls
    std::sort(std::begin(command_buffer_.entries), std::begin(command_buffer_.entries) + command_buffer_.count, 
        [&](const CommandBuffer::Entry& item1, const CommandBuffer::Entry& item2)
        {
        	return item1.first < item2.first;
        });
}

void RenderQueue::reset()
{
	command_buffer_.reset();
}

void RenderQueue::submit(const DrawCall& dc)
{
    W_PROFILE_RENDER_FUNCTION()

	W_ASSERT(dc.data.VAO.is_valid(), "Invalid VertexArrayHandle!");
	W_ASSERT(dc.data.shader.is_valid(), "Invalid ShaderHandle!");

	auto& cmdbuf = command_buffer_.storage;
	void* cmd = cmdbuf.head();

	cmdbuf.write(&dc.type);
	cmdbuf.write(&dc.data);
	if(dc.type == DrawCall::IndexedInstanced || dc.type == DrawCall::ArrayInstanced)
		cmdbuf.write(&dc.instance_data);

	command_buffer_.entries[command_buffer_.count++] = {dc.key.encode(), cmd};
}

// Helper function to identify which part of the pass state has changed
static inline bool has_mutated(uint64_t state, uint64_t old_state, uint64_t mask)
{
	return ((state^old_state)&mask) > 0;
}

static void handle_state(uint64_t state_flags)
{
	// * If pass state has changed, decode it, find which parts have changed and update device state
	static uint64_t last_state = PassState().encode(); // Initialized as default state
	if(state_flags != last_state)
	{
		PassState state;
		state.decode(state_flags);

		if(has_mutated(state_flags, last_state, k_framebuffer_mask))
		{
			if(state.render_target == s_storage.default_framebuffer_)
			{
				Gfx::device->bind_default_framebuffer();
				glm::vec2 vp_size = FramebufferPool::get_screen_size();
				Gfx::device->viewport(0, 0, vp_size.x, vp_size.y);
			}
			else
				s_storage.framebuffers[state.render_target.index]->bind();

			int clear_flags = ClearFlags::CLEAR_COLOR_FLAG
							| (state.depth_stencil_state.depth_test_enabled   ? ClearFlags::CLEAR_DEPTH_FLAG : ClearFlags::CLEAR_NONE)
							| (state.depth_stencil_state.stencil_test_enabled ? ClearFlags::CLEAR_STENCIL_FLAG : ClearFlags::CLEAR_NONE);
			Gfx::device->clear(clear_flags); // TMP: Ok for now, but this will not allow to blend the result of multiple passes
		}

		if(has_mutated(state_flags, last_state, k_cull_mode_mask))
			Gfx::device->set_cull_mode(state.rasterizer_state.cull_mode);
		
		if(has_mutated(state_flags, last_state, k_transp_mask))
		{
			switch(state.blend_state)
			{
				case BlendState::Alpha: Gfx::device->set_std_blending(); break;
				case BlendState::Light: Gfx::device->set_light_blending(); break;
				default:                Gfx::device->disable_blending(); break;
			}
		}

		if(has_mutated(state_flags, last_state, k_stencil_test_mask))
		{
			Gfx::device->set_stencil_test_enabled(state.depth_stencil_state.stencil_test_enabled);
			if(state.depth_stencil_state.stencil_test_enabled)
			{
				Gfx::device->set_stencil_func(state.depth_stencil_state.stencil_func);
				Gfx::device->set_stencil_operator(state.depth_stencil_state.stencil_operator);
			}
		}

		if(has_mutated(state_flags, last_state, k_depth_test_mask))
		{
			Gfx::device->set_depth_test_enabled(state.depth_stencil_state.depth_test_enabled);
			if(state.depth_stencil_state.depth_test_enabled)
				Gfx::device->set_depth_func(state.depth_stencil_state.depth_func);
		}
	
		last_state = state_flags;
	}
}

void Renderer::render_dispatch(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

    DrawCall::DrawCallType type;
    DrawCall::Data data;
	buf.read(&type);
	buf.read(&data); // Read all in one go

	handle_state(data.state_flags);

	// * Detect if a new shader needs to be used, update and bind shader resources
	static uint16_t last_shader_index = k_invalid_handle;
	static uint16_t last_texture_index[k_max_texture_slots];
	auto& shader = *s_storage.shaders[data.shader.index];
	if(data.shader.index != last_shader_index)
	{
		shader.bind();
		last_shader_index = data.shader.index;
		std::fill(last_texture_index, last_texture_index+k_max_texture_slots, k_invalid_handle);
	}
	uint32_t slot = 0;
	while(data.textures[slot].index != k_invalid_handle && slot < k_max_texture_slots) // Don't use is_valid() here, we only want to discriminate default initialized data
	{
		// Avoid texture switching if not necessary
		if(data.textures[slot].index != last_texture_index[slot])
		{
			auto& texture = *s_storage.textures[data.textures[slot].index];
			shader.attach_texture_2D(texture, slot);
			last_texture_index[slot] = data.textures[slot].index;
		}
		++slot;
	}
	slot = 0;
	while(data.UBOs[slot].index != k_invalid_handle && slot < k_max_UBO_slots) // Don't use is_valid() here, we only want to discriminate default initialized data
	{
		if(data.UBOs_data[slot])
		{
			auto& ubo = *s_storage.uniform_buffers[data.UBOs[slot].index];
			ubo.stream(data.UBOs_data[slot], 0, 0);
		}
		++slot;
	}

	// * Execute draw call
	static uint16_t last_VAO_index = k_invalid_handle;
	auto& va = *s_storage.vertex_arrays[data.VAO.index];
	// Avoid switching vertex array when possible
	if(data.VAO.index != last_VAO_index)
	{
		va.bind();
		last_VAO_index = data.VAO.index;
	}

	switch(type)
	{
		case DrawCall::Indexed:
			Gfx::device->draw_indexed(va, data.count, data.offset);
			break;
		case DrawCall::IndexedInstanced:
		{
			// Read additional data needed for instanced rendering
    		DrawCall::InstanceData idata;
			buf.read(&idata);
			if(idata.SSBO_data)
			{
				auto& ssbo = *s_storage.shader_storage_buffers[idata.SSBO.index];
				ssbo.stream(idata.SSBO_data, idata.SSBO_size, 0);
			}
			Gfx::device->draw_indexed_instanced(va, idata.instance_count, data.count, data.offset);
			break;
		}
		default:
			W_ASSERT(false, "Specified draw call type is unsupported at the moment.");
			break;
	}
}

void RenderQueue::flush()
{
    W_PROFILE_RENDER_FUNCTION()
    // Set clear color
    Gfx::device->set_clear_color(clear_color_.r, clear_color_.g, clear_color_.b, clear_color_.a);

    // Execute all draw commands in command buffer
	for(int ii=0; ii<command_buffer_.count; ++ii)
	{
		auto&& [key,cmd] = command_buffer_.entries[ii];
		command_buffer_.storage.seek(cmd);
		Renderer::render_dispatch(command_buffer_.storage);
	}
}

static void flush_command_buffer(CommandBuffer& cmdbuf)
{
    W_PROFILE_RENDER_FUNCTION()

    // Dispatch render commands in specified command buffer
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

static void sort_commands()
{
    W_PROFILE_RENDER_FUNCTION()
	// Keys stored separately from commands to avoid touching data too
	// much during sort calls
    std::sort(std::begin(s_storage.pre_buffer_.entries), std::begin(s_storage.pre_buffer_.entries) + s_storage.pre_buffer_.count, 
        [&](const CommandBuffer::Entry& item1, const CommandBuffer::Entry& item2)
        {
        	return item1.first < item2.first;
        });
    std::sort(std::begin(s_storage.post_buffer_.entries), std::begin(s_storage.post_buffer_.entries) + s_storage.post_buffer_.count, 
        [&](const CommandBuffer::Entry& item1, const CommandBuffer::Entry& item2)
        {
        	return item1.first < item2.first;
        });
}

void Renderer::flush()
{
    W_PROFILE_RENDER_FUNCTION()
    
	if(s_storage.profiling_enabled)
		s_storage.query_timer->start();

	// Sort command buffers
	sort_commands();
	// Dispatch pre buffer commands
	flush_command_buffer(s_storage.pre_buffer_);
	// Sort, flush and reset queue
	s_storage.queue_.sort();
	s_storage.queue_.flush();
	s_storage.queue_.reset();
	// Dispatch post buffer commands
	flush_command_buffer(s_storage.post_buffer_);
	// Reset auxiliary memory arena for next frame
	s_storage.auxiliary_arena_.reset();
	// Reset resource arena for next frame
	filesystem::reset_arena();

	// Execute callbacks
	for(auto&& callback: s_storage.end_frame_callbacks_)
		callback();

	if(s_storage.profiling_enabled)
	{
		auto render_duration = s_storage.query_timer->stop();
		s_storage.stats.render_time = std::chrono::duration_cast<std::chrono::microseconds>(render_duration).count();
	}
}

} // namespace erwin