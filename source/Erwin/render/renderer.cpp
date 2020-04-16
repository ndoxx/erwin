#include "render/renderer.h"

#include <map>
#include <memory>
#include <fstream>

#include "debug/logger.h"
#include "core/config.h"
#include "core/clock.hpp"
#include "memory/arena.h"
#include "memory/memory_utils.h"
#include "memory/handle_pool.h"
#include "memory/linear_allocator.h"
#include "memory/handle_pool.h"
#include "render/render_device.h"
#include "render/buffer.h"
#include "render/framebuffer.h"
#include "render/shader.h"
#include "render/query_timer.h"
#include "math/color.h"

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
		DO_ACTION( CubemapHandle )             \
		DO_ACTION( ShaderHandle )              \
		DO_ACTION( FramebufferHandle )

constexpr std::size_t k_handle_alloc_size = 10 * 2 * sizeof(HandlePoolT<k_max_render_handles>);

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
constexpr uint8_t  k_depth_bits      = 24;
constexpr uint8_t  k_seq_bits        = 24;
constexpr uint8_t  k_subseq_bits     = 8;
constexpr uint64_t k_view_shift      = uint8_t(64)        - k_view_bits;
constexpr uint64_t k_draw_type_shift = k_view_shift       - k_draw_type_bits;
constexpr uint64_t k_1_shader_shift  = k_draw_type_shift  - k_shader_bits;
constexpr uint64_t k_1_depth_shift   = k_1_shader_shift   - k_depth_bits;
constexpr uint64_t k_1_subseq_shift  = k_1_depth_shift    - k_subseq_bits;
constexpr uint64_t k_2_depth_shift   = k_draw_type_shift  - k_depth_bits;
constexpr uint64_t k_2_shader_shift  = k_2_depth_shift    - k_shader_bits;
constexpr uint64_t k_2_subseq_shift  = k_2_shader_shift   - k_subseq_bits;
constexpr uint64_t k_3_seq_shift     = k_draw_type_shift  - k_seq_bits;
constexpr uint64_t k_3_shader_shift  = k_3_seq_shift      - k_shader_bits;
constexpr uint64_t k_3_subseq_shift  = k_3_shader_shift   - k_subseq_bits;
constexpr uint64_t k_view_mask       = uint64_t(0x0000ffff) << k_view_shift;
constexpr uint64_t k_draw_type_mask  = uint64_t(0x00000003) << k_draw_type_shift;
constexpr uint64_t k_1_shader_mask   = uint64_t(0x000000ff) << k_1_shader_shift;
constexpr uint64_t k_1_depth_mask    = uint64_t(0x00ffffff) << k_1_depth_shift;
constexpr uint64_t k_1_subseq_mask   = uint64_t(0x000000ff) << k_1_subseq_shift;
constexpr uint64_t k_2_depth_mask    = uint64_t(0x00ffffff) << k_2_depth_shift;
constexpr uint64_t k_2_shader_mask   = uint64_t(0x000000ff) << k_2_shader_shift;
constexpr uint64_t k_2_subseq_mask   = uint64_t(0x000000ff) << k_2_subseq_shift;
constexpr uint64_t k_3_seq_mask      = uint64_t(0x00ffffff) << k_3_seq_shift;
constexpr uint64_t k_3_shader_mask   = uint64_t(0x000000ff) << k_3_shader_shift;
constexpr uint64_t k_3_subseq_mask   = uint64_t(0x000000ff) << k_3_subseq_shift;

uint64_t SortKey::encode() const
{
	uint64_t head = ((uint64_t(view)     << k_view_shift     ) & k_view_mask)
				  | ((uint64_t(blending) << k_draw_type_shift) & k_draw_type_mask);

	uint64_t body = 0;
	switch(order)
	{
		case SortKey::Order::ByShader:
		{
			body |= ((uint64_t(shader)       << k_1_shader_shift) & k_1_shader_mask)
				 |  ((uint64_t(depth)        << k_1_depth_shift)  & k_1_depth_mask)
				 |  ((uint64_t(sub_sequence) << k_1_subseq_shift) & k_1_subseq_mask);
			break;
		}
		case SortKey::Order::ByDepthDescending:
		{
			body |= ((uint64_t(depth)        << k_2_depth_shift)  & k_2_depth_mask)
				 |  ((uint64_t(shader)       << k_2_shader_shift) & k_2_shader_mask)
				 |  ((uint64_t(sub_sequence) << k_2_subseq_shift) & k_2_subseq_mask);
			break;
		}
		case SortKey::Order::ByDepthAscending:
		{
			body |= ((uint64_t(~depth)       << k_2_depth_shift)  & k_2_depth_mask)
				 |  ((uint64_t(shader)       << k_2_shader_shift) & k_2_shader_mask)
				 |  ((uint64_t(sub_sequence) << k_2_subseq_shift) & k_2_subseq_mask);
			break;
		}
		case SortKey::Order::Sequential:
		{
			body |= ((uint64_t(sequence)     << k_3_seq_shift)    & k_3_seq_mask)
				 |  ((uint64_t(shader)       << k_3_shader_shift) & k_3_shader_mask)
				 |  ((uint64_t(sub_sequence) << k_3_subseq_shift) & k_3_subseq_mask);
			break;
		}
	}

	return head | body;
}



/*
		  _____                _              ____                        
		 |  __ \              | |            / __ \                       
		 | |__) |___ _ __   __| | ___ _ __  | |  | |_   _  ___ _   _  ___ 
		 |  _  // _ \ '_ \ / _` |/ _ \ '__| | |  | | | | |/ _ \ | | |/ _ \
		 | | \ \  __/ | | | (_| |  __/ |    | |__| | |_| |  __/ |_| |  __/
		 |_|  \_\___|_| |_|\__,_|\___|_|     \___\_\\__,_|\___|\__,_|\___|
*/

template<std::size_t SIZE>
struct CommandBuffer
{
	typedef std::pair<uint64_t,void*> Entry;

	CommandBuffer() = default;
	CommandBuffer(memory::HeapArea& area, std::size_t size, const char* debug_name)
	{
		init(area, size, debug_name);
	}

	inline void init(memory::HeapArea& area, std::size_t size, const char* debug_name)
	{
		count = 0;
		storage.init(area, size, debug_name);
	}

	inline void reset()
	{
		storage.reset();
		count = 0;
	}

	inline bool is_full() const
	{
		return count >= SIZE;
	}

	std::size_t count;
	memory::LinearBuffer<> storage;
	Entry entries[SIZE];
};

using RenderCommandBuffer = CommandBuffer<k_max_render_commands>;
using DrawCommandBuffer   = CommandBuffer<k_max_draw_calls>;

class RenderQueue
{
public:
	friend class Renderer;
	friend class DrawCommandWriter;

	RenderQueue() = default;
	RenderQueue(memory::HeapArea& area);
	~RenderQueue();

	void init(memory::HeapArea& area);

	// * These functions change the queue state persistently
	// Set clear color for this queue
	inline void set_clear_color(const glm::vec4& clear_color) { clear_color_ = clear_color; }
	// Check if the queue is full
	inline bool is_full() const { return command_buffer_.is_full(); }
	// Sort queue by sorting key
	void sort();
	// Dispatch all commands
	void flush();
	// Clear queue
	void reset();

private:
	uint8_t current_view_id_;
	glm::vec4 clear_color_;
	DrawCommandBuffer command_buffer_;
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
	current_view_id_ = 0;
}

void RenderQueue::sort()
{
    W_PROFILE_RENDER_FUNCTION()

	// Keys stored separately from commands to avoid touching data too
	// much during sort calls
    std::sort(std::begin(command_buffer_.entries), std::begin(command_buffer_.entries) + command_buffer_.count, 
        [&](const DrawCommandBuffer::Entry& item1, const DrawCommandBuffer::Entry& item2)
        {
        	return item1.first < item2.first;
        });
}

void RenderQueue::reset()
{
	command_buffer_.reset();
	current_view_id_ = 0;
}

#if W_RC_PROFILE_DRAW_CALLS
// Allows to track draw calls from submission to dispatch during a single frame
struct FrameDrawCallData
{
	struct DrawCallSummary
	{
		uint64_t render_state;
		DrawCall::DrawCallType type;
		uint16_t shader_handle;
		uint32_t submission_index;
	};

	inline void on_submit(const DrawCall& dc, uint64_t key)
	{
		draw_calls.insert(std::make_pair(key,
			DrawCallSummary
			{
				dc.data.state_flags,
				dc.type,
				dc.data.shader.index,
				submitted++
			}));
	}

	inline void on_dispatch(uint64_t)
	{
		// Nothing to do for now, map ordering is the same as queue ordering once sorted
	}

	inline void reset()
	{
		draw_calls.clear();
		tracking = false;
		submitted = 0;
	}

	void export_json();

	std::multimap<uint64_t, DrawCallSummary> draw_calls;
	fs::path json_path;
	bool tracking = false;
	uint32_t submitted = 0;
};
#endif

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

static int FRONT = 0;
static int BACK = 1;

struct FramebufferTextureVector
{
	std::vector<TextureHandle> handles;
	std::vector<hash_t> debug_names;
};

struct ShaderCompatibility
{
	bool ready = false;
	BufferLayout layout;

	inline void set_layout(const BufferLayout& _layout)
	{
		layout = _layout;
		ready = true;
	}

	inline void clear()
	{
		layout.clear();
		ready = false;
	}
};

static struct RendererStorage
{
	RendererStorage(): initialized_(false) {}
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
		std::fill(std::begin(cubemaps),               std::end(cubemaps),               nullptr);
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

		state_cache_ = RenderState().encode(); // Initialized as default state
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

	bool initialized_;

	// TODO: Drop WRefs and use arenas (with pool allocator?) to allocate memory for these objects
	WRef<IndexBuffer>         index_buffers[k_max_render_handles];
	WRef<BufferLayout>        vertex_buffer_layouts[k_max_render_handles];
	WRef<VertexBuffer>        vertex_buffers[k_max_render_handles];
	WRef<VertexArray>         vertex_arrays[k_max_render_handles];
	WRef<UniformBuffer>       uniform_buffers[k_max_render_handles];
	WRef<ShaderStorageBuffer> shader_storage_buffers[k_max_render_handles];
	WRef<Texture2D>			  textures[k_max_render_handles];
	WRef<Cubemap>			  cubemaps[k_max_render_handles];
	WRef<Shader>			  shaders[k_max_render_handles];
	WRef<Framebuffer>		  framebuffers[k_max_render_handles];

	ShaderCompatibility shader_compat[k_max_render_handles];

	FramebufferHandle default_framebuffer_;
	FramebufferHandle current_framebuffer_;
	std::map<uint16_t, FramebufferTextureVector> framebuffer_textures_;
	uint64_t state_cache_;
	glm::vec2 host_window_size_;

	WScope<QueryTimer> query_timer;
	bool profiling_enabled;
	Renderer::Statistics stats[2]; // Double buffered

#if W_RC_PROFILE_DRAW_CALLS
	FrameDrawCallData draw_call_data;
#endif

	memory::HeapArea* renderer_memory_;
	RenderCommandBuffer pre_buffer_;
	RenderCommandBuffer post_buffer_;
	Renderer::AuxArena auxiliary_arena_;
	LinearArena handle_arena_;
	RenderQueue queue_;
} s_storage;


#if W_RC_PROFILE_DRAW_CALLS
void FrameDrawCallData::export_json()
{
	DLOGN("render") << "Exporting frame draw call profile:" << std::endl;
	DLOGI << WCC('p') << json_path << std::endl;

	std::ofstream ofs(json_path);

	ofs << "{"
		<< "\"draw_calls\":[" << std::endl;

	// We assume map ordering is the same as queue ordering, elements are thus
	// presented in dispatch order
	uint32_t count = 0;
	for(auto&& [key,summary]: draw_calls)
	{
		RenderState render_state;
		render_state.decode(summary.render_state);
    	ofs << "{"
    		<< "\"key\":" << key << ","
    		<< "\"sub\":" << summary.submission_index << ","
    		<< "\"typ\":" << int(summary.type) << ","
    		<< "\"shd\":\"" << s_storage.shaders[summary.shader_handle]->get_name() << "\","
    		<< "\"sta\":\"" << render_state.to_string() << "\""
            << ((count<submitted-1) ? "}," : "}") << std::endl;

        ++count;
    }

	ofs << "]}" << std::endl;

	ofs.close();
	reset();

	DLOG("render",1) << "done" << std::endl;
}
#endif


void Renderer::init(memory::HeapArea& area)
{
    W_PROFILE_RENDER_FUNCTION()

    if(s_storage.initialized_)
    {
    	DLOGW("render") << "[Renderer] Already initialized, skipping." << std::endl;
    	return;
    }

	DLOGN("render") << "[Renderer] Allocating renderer storage." << std::endl;
	
	// Create and initialize storage object
	s_storage.init(&area);
	s_storage.query_timer = QueryTimer::create();
	s_storage.profiling_enabled = false;
	s_storage.default_framebuffer_ = FramebufferHandle::acquire();
	s_storage.current_framebuffer_ = s_storage.default_framebuffer_;
	s_storage.host_window_size_ = {0, 0};

	DLOGI << "done" << std::endl;

	DLOGN("render") << "[Renderer] Creating main render targets." << std::endl;
	// Create main render targets
	{
	    FramebufferLayout layout
	    {
	    	// RGB: Albedo, A: Scaled emissivity
	        {"albedo"_h, ImageFormat::RGBA16F, MIN_NEAREST | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE},
	        // RG: Compressed normal, BA: ?
	        {"normal"_h, ImageFormat::RGBA16_SNORM, MIN_NEAREST | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE},
	        // R: Metallic, G: AO, B: Roughness, A: ?
	        {"mar"_h,    ImageFormat::RGBA8, MIN_NEAREST | MAG_LINEAR, TextureWrap::CLAMP_TO_EDGE},
	    };
	    FramebufferPool::create_framebuffer("GBuffer"_h, make_scope<FbRatioConstraint>(), layout, true, true);
	}
	{
	    FramebufferLayout layout
	    {
	    	// RGBA: HDR color
	        {"albedo"_h, ImageFormat::RGBA16F, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE},
	        // RGB: Glow color, A: Glow intensity
	        {"glow"_h,   ImageFormat::RGBA8, MIN_LINEAR | MAG_LINEAR, TextureWrap::CLAMP_TO_EDGE},
	    };
	    FramebufferPool::create_framebuffer("LBuffer"_h, make_scope<FbRatioConstraint>(), layout, true, true);
	}
	/*{
		// Debug render target
	    FramebufferLayout layout =
	    {
	        {"target_0"_h, ImageFormat::RGBA8, MIN_NEAREST | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE},
	        {"target_1"_h, ImageFormat::RGBA8, MIN_NEAREST | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE},
	    };
	    FramebufferPool::create_framebuffer("DBuffer"_h, make_scope<FbRatioConstraint>(), layout, false);
	}*/

	s_storage.initialized_ = true;

	DLOGI << "done" << std::endl;
}

void Renderer::shutdown()
{
    W_PROFILE_RENDER_FUNCTION()

    if(!s_storage.initialized_)
    {
    	DLOGW("render") << "[Renderer] Not initialized, skipping shutdown." << std::endl;
    	return;
    }

	flush();
	DLOGN("render") << "[Renderer] Releasing renderer storage." << std::endl;
	s_storage.release();
	s_storage.initialized_ = false;

	DLOGI << "done" << std::endl;
}

uint8_t Renderer::next_layer_id()
{
	W_ASSERT(s_storage.queue_.current_view_id_<255, "View id overflow.");
	return s_storage.queue_.current_view_id_++;
}

Renderer::AuxArena& Renderer::get_arena()
{
	return s_storage.auxiliary_arena_;
}

#ifdef W_DEBUG
void Renderer::set_profiling_enabled(bool value)
{
	s_storage.profiling_enabled = value;
}

const Renderer::Statistics& Renderer::get_stats()
{
	return s_storage.stats[BACK];
}
#endif

void Renderer::track_draw_calls(const fs::path& json_path)
{
#if W_RC_PROFILE_DRAW_CALLS
	s_storage.draw_call_data.json_path = json_path;
	s_storage.draw_call_data.tracking = true;
#endif
}

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

hash_t Renderer::get_framebuffer_texture_name(FramebufferHandle handle, uint32_t index)
{
	W_ASSERT(handle.is_valid(), "Invalid FramebufferHandle.");
	return s_storage.framebuffer_textures_[handle.index].debug_names[index];
}

uint32_t Renderer::get_framebuffer_texture_count(FramebufferHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid FramebufferHandle.");
	return uint32_t(s_storage.framebuffer_textures_[handle.index].handles.size());
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

	s_storage.vertex_buffer_layouts[handle.index] = make_ref<BufferLayout>(const_cast<BufferLayoutElement*>(elements.data()), elements.size());

	return handle;
}

const BufferLayout& Renderer::get_vertex_buffer_layout(VertexBufferLayoutHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid VertexBufferLayoutHandle!");

	return *s_storage.vertex_buffer_layouts[handle.index];
}

bool Renderer::is_compatible(VertexBufferLayoutHandle layout, ShaderHandle shader)
{
	W_ASSERT(layout.is_valid(), "Invalid VertexBufferLayoutHandle!");
	W_ASSERT(shader.is_valid(), "Invalid ShaderHandle!");

	if(!s_storage.shader_compat[shader.index].ready)
		return false;

	return s_storage.vertex_buffer_layouts[layout.index]->compare(s_storage.shader_compat[shader.index].layout);
}


/*
		   _____                                          _     
		  / ____|                                        | |    
		 | |     ___  _ __ ___  _ __ ___   __ _ _ __   __| |___ 
		 | |    / _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` / __|
		 | |___| (_) | | | | | | | | | | | (_| | | | | (_| \__ \
		  \_____\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_|___/
*/

// --- Render commands ---

enum class RenderCommand: uint16_t
{
	CreateIndexBuffer,
	CreateVertexBuffer,
	CreateVertexArray,
	CreateVertexArrayMultipleVBO,
	CreateUniformBuffer,
	CreateShaderStorageBuffer,
	CreateShader,
	CreateTexture2D,
	CreateCubemap,
	CreateFramebuffer,

	UpdateIndexBuffer,
	UpdateVertexBuffer,
	UpdateUniformBuffer,
	UpdateShaderStorageBuffer,
	ShaderAttachUniformBuffer,
	ShaderAttachStorageBuffer,
	UpdateFramebuffer,
	ClearFramebuffers,
	SetHostWindowSize,

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
	DestroyCubemap,
	DestroyFramebuffer,

	Count
};

// Helper class for command buffer access
class RenderCommandWriter
{
public:
	RenderCommandWriter(RenderCommand type):
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
		W_ASSERT_FMT(!cmdbuf_.is_full(), "Command buffer %d is full!", int(type_));
		uint64_t key = uint64_t(cmdbuf_.count);
		cmdbuf_.entries[cmdbuf_.count++] = {key, head_};
	}

private:
	enum class Phase
	{
		Pre,
		Post
	};

	inline RenderCommandBuffer& get_command_buffer(Phase phase)
	{
		switch(phase)
		{
			case Phase::Pre:  return s_storage.pre_buffer_;
			case Phase::Post: return s_storage.post_buffer_;
		}
	}

	inline RenderCommandBuffer& get_command_buffer(RenderCommand command)
	{
		Phase phase = (command < RenderCommand::Post) ? Phase::Pre : Phase::Post;
		return get_command_buffer(phase);
	}

private:
	RenderCommand type_;
	RenderCommandBuffer& cmdbuf_;
	void* head_;
};

IndexBufferHandle Renderer::create_index_buffer(const uint32_t* index_data, uint32_t count, DrawPrimitive primitive, UsagePattern mode)
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
		W_ASSERT(mode != UsagePattern::Static, "Index data can't be null in static mode.");

	// Write data
	RenderCommandWriter cw(RenderCommand::CreateIndexBuffer);
	cw.write(&handle);
	cw.write(&count);
	cw.write(&primitive);
	cw.write(&mode);
	cw.write(&auxiliary);
	cw.submit();

	return handle;
}

VertexBufferHandle Renderer::create_vertex_buffer(VertexBufferLayoutHandle layout, const float* vertex_data, uint32_t count, UsagePattern mode)
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
		W_ASSERT(mode != UsagePattern::Static, "Vertex data can't be null in static mode.");

	// Write data
	RenderCommandWriter cw(RenderCommand::CreateVertexBuffer);
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

	VertexArrayHandle handle = VertexArrayHandle::acquire();
	W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

	// Write data
	RenderCommandWriter cw(RenderCommand::CreateVertexArray);
	cw.write(&handle);
	cw.write(&ib);
	cw.write(&vb);
	cw.submit();

	return handle;
}

VertexArrayHandle Renderer::create_vertex_array(const std::vector<VertexBufferHandle>& vbs, IndexBufferHandle ib)
{
	VertexArrayHandle handle = VertexArrayHandle::acquire();
	W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

	uint8_t VBO_count = uint8_t(vbs.size());

	RenderCommandWriter cw(RenderCommand::CreateVertexArrayMultipleVBO);
	cw.write(&handle);
	cw.write(&ib);
	cw.write(&VBO_count);

	for(auto vb: vbs)
	{
		W_ASSERT_FMT(vb.is_valid(), "Invalid VertexBufferHandle: %hu.", vb.index);
		cw.write(&vb);
	}
	cw.submit();

	return handle;
}

UniformBufferHandle Renderer::create_uniform_buffer(const std::string& name, void* data, uint32_t size, UsagePattern mode)
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
		W_ASSERT(mode != UsagePattern::Static, "UBO data can't be null in static mode.");

	// Write data
	RenderCommandWriter cw(RenderCommand::CreateUniformBuffer);
	cw.write(&handle);
	cw.write(&size);
	cw.write(&mode);
	cw.write_str(name);
	cw.write(&auxiliary);
	cw.submit();

	return handle;
}

ShaderStorageBufferHandle Renderer::create_shader_storage_buffer(const std::string& name, void* data, uint32_t size, UsagePattern mode)
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
		W_ASSERT(mode != UsagePattern::Static, "SSBO data can't be null in static mode.");

	// Write data
	RenderCommandWriter cw(RenderCommand::CreateShaderStorageBuffer);
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

	RenderCommandWriter cw(RenderCommand::CreateShader);
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

	RenderCommandWriter cw(RenderCommand::CreateTexture2D);
	cw.write(&handle);
	cw.write(&desc);
	cw.submit();

	return handle;
}

CubemapHandle Renderer::create_cubemap(const CubemapDescriptor& desc)
{
	CubemapHandle handle = CubemapHandle::acquire();
	W_ASSERT(handle.is_valid(), "No more free handle in handle pool.");

	RenderCommandWriter cw(RenderCommand::CreateCubemap);
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
	uint32_t tex_count = depth ? uint32_t(layout.get_count())+1 : uint32_t(layout.get_count()); // Take the depth texture into account
	for(uint32_t ii=0; ii<tex_count; ++ii)
	{
		TextureHandle tex_handle = TextureHandle::acquire();
		W_ASSERT(tex_handle.is_valid(), "No more free handle in handle pool.");
		texture_vector.handles.push_back(tex_handle);
		texture_vector.debug_names.push_back((depth && ii==tex_count-1) ? "depth"_h : layout[ii].target_name);
	}
	s_storage.framebuffer_textures_.insert(std::make_pair(handle.index, texture_vector));
	uint32_t count = uint32_t(layout.get_count());

	// Allocate auxiliary data
	FramebufferLayoutElement* auxiliary = W_NEW_ARRAY_DYNAMIC(FramebufferLayoutElement, count, s_storage.auxiliary_arena_);
	memcpy(auxiliary, layout.data(), count * sizeof(FramebufferLayoutElement));

	RenderCommandWriter cw(RenderCommand::CreateFramebuffer);
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

	RenderCommandWriter cw(RenderCommand::UpdateIndexBuffer);
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

	RenderCommandWriter cw(RenderCommand::UpdateVertexBuffer);
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

	RenderCommandWriter cw(RenderCommand::UpdateUniformBuffer);
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

	RenderCommandWriter cw(RenderCommand::UpdateShaderStorageBuffer);
	cw.write(&handle);
	cw.write(&size);
	cw.write(&auxiliary);
	cw.submit();
}

void Renderer::shader_attach_uniform_buffer(ShaderHandle shader, UniformBufferHandle ubo)
{
	W_ASSERT(shader.is_valid(), "Invalid ShaderHandle!");
	W_ASSERT(ubo.is_valid(), "Invalid UniformBufferHandle!");

	RenderCommandWriter cw(RenderCommand::ShaderAttachUniformBuffer);
	cw.write(&shader);
	cw.write(&ubo);
	cw.submit();
}

void Renderer::shader_attach_storage_buffer(ShaderHandle shader, ShaderStorageBufferHandle ssbo)
{
	W_ASSERT(shader.is_valid(), "Invalid ShaderHandle!");
	W_ASSERT(ssbo.is_valid(), "Invalid ShaderStorageBufferHandle!");

	RenderCommandWriter cw(RenderCommand::ShaderAttachStorageBuffer);
	cw.write(&shader);
	cw.write(&ssbo);
	cw.submit();
}

void Renderer::update_framebuffer(FramebufferHandle fb, uint32_t width, uint32_t height)
{
	W_ASSERT(fb.is_valid(), "Invalid FramebufferHandle!");

	RenderCommandWriter cw(RenderCommand::UpdateFramebuffer);
	cw.write(&fb);
	cw.write(&width);
	cw.write(&height);
	cw.submit();
}

void Renderer::clear_framebuffers()
{
	RenderCommandWriter cw(RenderCommand::ClearFramebuffers);
	cw.submit();
}

void Renderer::set_host_window_size(uint32_t width, uint32_t height)
{
	RenderCommandWriter cw(RenderCommand::SetHostWindowSize);
	cw.write(&width);
	cw.write(&height);
	cw.submit();
}

void Renderer::framebuffer_screenshot(FramebufferHandle fb, const fs::path& filepath)
{
	W_ASSERT(fb.is_valid(), "Invalid FramebufferHandle!");

	RenderCommandWriter cw(RenderCommand::FramebufferScreenshot);
	cw.write(&fb);
	cw.write_str(filepath.string());
	cw.submit();
}

void Renderer::destroy(IndexBufferHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid IndexBufferHandle!");

	RenderCommandWriter cw(RenderCommand::DestroyIndexBuffer);
	cw.write(&handle);
	cw.submit();
}

void Renderer::destroy(VertexBufferLayoutHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid VertexBufferLayoutHandle!");

	RenderCommandWriter cw(RenderCommand::DestroyVertexBufferLayout);
	cw.write(&handle);
	cw.submit();
}

void Renderer::destroy(VertexBufferHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid VertexBufferHandle!");

	RenderCommandWriter cw(RenderCommand::DestroyVertexBuffer);
	cw.write(&handle);
	cw.submit();
}

void Renderer::destroy(VertexArrayHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid VertexArrayHandle!");

	RenderCommandWriter cw(RenderCommand::DestroyVertexArray);
	cw.write(&handle);
	cw.submit();
}

void Renderer::destroy(UniformBufferHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid UniformBufferHandle!");

	RenderCommandWriter cw(RenderCommand::DestroyUniformBuffer);
	cw.write(&handle);
	cw.submit();
}

void Renderer::destroy(ShaderStorageBufferHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid ShaderStorageBufferHandle!");

	RenderCommandWriter cw(RenderCommand::DestroyShaderStorageBuffer);
	cw.write(&handle);
	cw.submit();
}

void Renderer::destroy(ShaderHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid ShaderHandle!");
	
	RenderCommandWriter cw(RenderCommand::DestroyShader);
	cw.write(&handle);
	cw.submit();
}

void Renderer::destroy(TextureHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid TextureHandle!");
	
	RenderCommandWriter cw(RenderCommand::DestroyTexture2D);
	cw.write(&handle);
	cw.submit();
}

void Renderer::destroy(CubemapHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid CubemapHandle!");
	
	RenderCommandWriter cw(RenderCommand::DestroyCubemap);
	cw.write(&handle);
	cw.submit();
}

void Renderer::destroy(FramebufferHandle handle)
{
	W_ASSERT(handle.is_valid(), "Invalid FramebufferHandle!");

	RenderCommandWriter cw(RenderCommand::DestroyFramebuffer);
	cw.write(&handle);
	cw.submit();
}


// --- Draw commands ---

enum class DrawCommand: uint16_t
{
	Draw,
	Clear,
	BlitDepth,
	UpdateShaderStorageBuffer,
	UpdateUniformBuffer,

	Count
};

// Helper class for draw command buffer access
class DrawCommandWriter
{
public:
	DrawCommandWriter(DrawCommand type):
	type_(type),
	cmdbuf_(s_storage.queue_.command_buffer_),
	head_(cmdbuf_.storage.head())
	{
		cmdbuf_.storage.write(&type_);
	}

	template <typename T>
	inline void write(T* source)                  { cmdbuf_.storage.write(source); }
	inline void write_str(const std::string& str) { cmdbuf_.storage.write_str(str); }

	inline uint32_t submit(uint64_t key)
	{
		W_ASSERT(!cmdbuf_.is_full(), "Render queue is full!");
		cmdbuf_.entries[cmdbuf_.count] = {key, head_};
		return uint32_t(cmdbuf_.count++);
	}

private:
	DrawCommand type_;
	DrawCommandBuffer& cmdbuf_;
	void* head_;
};

void Renderer::submit(uint64_t key, const DrawCall& dc)
{
#if W_RC_PROFILE_DRAW_CALLS
	if(s_storage.draw_call_data.tracking)
		s_storage.draw_call_data.on_submit(dc, key);
#endif

	DrawCommandWriter cw(DrawCommand::Draw);

	// Handle dependencies
	cw.write(&dc.dependency_count);
	for(uint8_t ii=0; ii<dc.dependency_count; ++ii)
	{
		void* ptr = s_storage.queue_.command_buffer_.entries[dc.dependencies[ii]].second;
		cw.write(&ptr);
	}

	cw.write(&dc.type);
	cw.write(&dc.data);
	cw.write(&dc.texture_count);
	for(uint8_t ii=0; ii<dc.texture_count; ++ii)
		cw.write(&dc.textures[ii]);
	cw.write(&dc.cubemap_count);
	for(uint8_t ii=0; ii<dc.cubemap_count; ++ii)
		cw.write(&dc.cubemaps[ii]);
	if(dc.type == DrawCall::IndexedInstanced || dc.type == DrawCall::ArrayInstanced)
		cw.write(&dc.instance_count);

	if(s_storage.profiling_enabled)
		++s_storage.stats[FRONT].draw_call_count;

	cw.submit(key);
}

void Renderer::clear(uint64_t key, FramebufferHandle target, uint32_t flags, const glm::vec4& clear_color)
{
	W_ASSERT_FMT(target.is_valid(), "Invalid FramebufferHandle: %hu", target.index);

	uint32_t color = color::pack(clear_color);

	DrawCommandWriter cw(DrawCommand::Clear);
	cw.write(&target);
	cw.write(&flags);
	cw.write(&color);

	cw.submit(key);
}

void Renderer::blit_depth(uint64_t key, FramebufferHandle source, FramebufferHandle target)
{
	W_ASSERT_FMT(source.is_valid(), "Invalid FramebufferHandle: %hu", source.index);
	W_ASSERT_FMT(target.is_valid(), "Invalid FramebufferHandle: %hu", target.index);

	DrawCommandWriter cw(DrawCommand::BlitDepth);
	cw.write(&source);
	cw.write(&target);

	cw.submit(key);
}

uint32_t Renderer::update_shader_storage_buffer(ShaderStorageBufferHandle handle, void* data, uint32_t size, DataOwnership copy)
{
	W_ASSERT_FMT(handle.is_valid(), "Invalid ShaderStorageBufferHandle: %hu", handle.index);
	W_ASSERT(data, "Data is null.");

	DrawCommandWriter cw(DrawCommand::UpdateShaderStorageBuffer);
	cw.write(&handle);
	cw.write(&size);

	if(data && bool(copy))
	{
		void* data_copy = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage.auxiliary_arena_);
		memcpy(data_copy, data, size);
		cw.write(&data_copy);
	}
	else
		cw.write(&data);

	return cw.submit(SortKey::k_skip);
}

uint32_t Renderer::update_uniform_buffer(UniformBufferHandle handle, void* data, uint32_t size, DataOwnership copy)
{
	W_ASSERT_FMT(handle.is_valid(), "Invalid UniformBufferHandle: %hu", handle.index);
	W_ASSERT(data, "Data is null.");

	DrawCommandWriter cw(DrawCommand::UpdateUniformBuffer);
	cw.write(&handle);
	cw.write(&size);

	if(data && bool(copy))
	{
		void* data_copy = W_NEW_ARRAY_DYNAMIC(uint8_t, size, s_storage.auxiliary_arena_);
		memcpy(data_copy, data, size);
		cw.write(&data_copy);
	}
	else
		cw.write(&data);

	return cw.submit(SortKey::k_skip);
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

namespace render_dispatch
{

void create_index_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	IndexBufferHandle handle;
	uint32_t count;
	DrawPrimitive primitive;
	UsagePattern mode;
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
	UsagePattern mode;
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
	IndexBufferHandle ib;
	VertexBufferHandle vb;
	buf.read(&handle);
	buf.read(&ib);
	buf.read(&vb);

	s_storage.vertex_arrays[handle.index] = VertexArray::create();
	s_storage.vertex_arrays[handle.index]->set_vertex_buffer(s_storage.vertex_buffers[vb.index]);
	if(ib.index != k_invalid_handle)
		s_storage.vertex_arrays[handle.index]->set_index_buffer(s_storage.index_buffers[ib.index]);
}

void create_vertex_array_multiple_VBO(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	VertexArrayHandle handle;
	IndexBufferHandle ib;
	uint8_t VBO_count;
	buf.read(&handle);
	buf.read(&ib);
	buf.read(&VBO_count);

	s_storage.vertex_arrays[handle.index] = VertexArray::create();

	for(uint8_t ii=0; ii<VBO_count; ++ii)
	{
		VertexBufferHandle vb;
		buf.read(&vb);
		s_storage.vertex_arrays[handle.index]->add_vertex_buffer(s_storage.vertex_buffers[vb.index]);
	}

	if(ib.index != k_invalid_handle)
		s_storage.vertex_arrays[handle.index]->set_index_buffer(s_storage.index_buffers[ib.index]);
}

void create_uniform_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	UniformBufferHandle handle;
	uint32_t size;
	UsagePattern mode;
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
	UsagePattern mode;
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

	s_storage.shaders[handle.index] = Shader::create(name, fs::path(filepath));
	s_storage.shader_compat[handle.index].set_layout(s_storage.shaders[handle.index]->get_attribute_layout());
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

void create_cubemap(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	CubemapHandle handle;
	CubemapDescriptor descriptor;
	buf.read(&handle);
	buf.read(&descriptor);

	s_storage.cubemaps[handle.index] = Cubemap::create(descriptor);
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
		s_storage.textures[texture_vector.handles[ii].index] = std::static_pointer_cast<Texture2D>(fb->get_shared_texture(ii));
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

	s_storage.index_buffers[handle.index]->map(auxiliary, count*sizeof(uint32_t));
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

	auto& UBO = *s_storage.uniform_buffers[handle.index];
	UBO.map(auxiliary, size ? size : UBO.get_size());
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
	shader.attach_uniform_buffer(*s_storage.uniform_buffers[ubo_handle.index]);
}

void shader_attach_storage_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	ShaderHandle shader_handle;
	ShaderStorageBufferHandle ssbo_handle;
	buf.read(&shader_handle);
	buf.read(&ssbo_handle);

	auto& shader = *s_storage.shaders[shader_handle.index];
	shader.attach_shader_storage(*s_storage.shader_storage_buffers[ssbo_handle.index]);
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
	bool has_cubemap = s_storage.framebuffers[fb_handle.index]->has_cubemap();
	auto layout      = s_storage.framebuffers[fb_handle.index]->get_layout();

	if(!has_cubemap)
	{
		s_storage.framebuffers[fb_handle.index] = Framebuffer::create(width, height, layout, has_depth, has_stencil);

		// Update framebuffer textures
		auto& fb = s_storage.framebuffers[fb_handle.index];
		auto& texture_vector = s_storage.framebuffer_textures_[fb_handle.index];
		for(uint32_t ii=0; ii<texture_vector.handles.size(); ++ii)
			s_storage.textures[texture_vector.handles[ii].index] = std::static_pointer_cast<Texture2D>(fb->get_shared_texture(ii));
	}
	else
	{
		W_ASSERT(false, "Cubemap attachment not supported yet.");
	}
}

void clear_framebuffers(memory::LinearBuffer<>&)
{
	FramebufferPool::traverse_framebuffers([](FramebufferHandle handle)
	{
		s_storage.framebuffers[handle.index]->bind();
		Gfx::device->clear(ClearFlags::CLEAR_COLOR_FLAG | ClearFlags::CLEAR_DEPTH_FLAG);
	});
}

void set_host_window_size(memory::LinearBuffer<>& buf)
{
	uint32_t width;
	uint32_t height;
	buf.read(&width);
	buf.read(&height);

	s_storage.host_window_size_ = {width, height};
}

void nop(memory::LinearBuffer<>&) { }

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
	handle.release();
}

void destroy_vertex_buffer_layout(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	VertexBufferLayoutHandle handle;
	buf.read(&handle);
	s_storage.vertex_buffer_layouts[handle.index] = nullptr;
	handle.release();
}

void destroy_vertex_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	VertexBufferHandle handle;
	buf.read(&handle);
	s_storage.vertex_buffers[handle.index] = nullptr;
	handle.release();
}

void destroy_vertex_array(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	VertexArrayHandle handle;
	buf.read(&handle);
	s_storage.vertex_arrays[handle.index] = nullptr;
	handle.release();
}

void destroy_uniform_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	UniformBufferHandle handle;
	buf.read(&handle);
	s_storage.uniform_buffers[handle.index] = nullptr;
	handle.release();
}

void destroy_shader_storage_buffer(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	ShaderStorageBufferHandle handle;
	buf.read(&handle);
	s_storage.shader_storage_buffers[handle.index] = nullptr;
	handle.release();
}

void destroy_shader(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	ShaderHandle handle;
	buf.read(&handle);
	s_storage.shaders[handle.index] = nullptr;
	s_storage.shader_compat[handle.index].clear();
	handle.release();
}

void destroy_texture_2D(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	TextureHandle handle;
	buf.read(&handle);
	s_storage.textures[handle.index] = nullptr;
	handle.release();
}

void destroy_cubemap(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

	CubemapHandle handle;
	buf.read(&handle);
	s_storage.cubemaps[handle.index] = nullptr;
	handle.release();
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
	handle.release();
}

} // namespace render_dispatch

typedef void (* backend_dispatch_func_t)(memory::LinearBuffer<>&);
static backend_dispatch_func_t render_backend_dispatch[std::size_t(RenderCommand::Count)] =
{
	&render_dispatch::create_index_buffer,
	&render_dispatch::create_vertex_buffer,
	&render_dispatch::create_vertex_array,
	&render_dispatch::create_vertex_array_multiple_VBO,
	&render_dispatch::create_uniform_buffer,
	&render_dispatch::create_shader_storage_buffer,
	&render_dispatch::create_shader,
	&render_dispatch::create_texture_2D,
	&render_dispatch::create_cubemap,
	&render_dispatch::create_framebuffer,
	&render_dispatch::update_index_buffer,
	&render_dispatch::update_vertex_buffer,
	&render_dispatch::update_uniform_buffer,
	&render_dispatch::update_shader_storage_buffer,
	&render_dispatch::shader_attach_uniform_buffer,
	&render_dispatch::shader_attach_storage_buffer,
	&render_dispatch::update_framebuffer,
	&render_dispatch::clear_framebuffers,
	&render_dispatch::set_host_window_size,

	&render_dispatch::nop,

	&render_dispatch::framebuffer_screenshot,
	&render_dispatch::destroy_index_buffer,
	&render_dispatch::destroy_vertex_buffer_layout,
	&render_dispatch::destroy_vertex_buffer,
	&render_dispatch::destroy_vertex_array,
	&render_dispatch::destroy_uniform_buffer,
	&render_dispatch::destroy_shader_storage_buffer,
	&render_dispatch::destroy_shader,
	&render_dispatch::destroy_texture_2D,
	&render_dispatch::destroy_cubemap,
	&render_dispatch::destroy_framebuffer,
};

// Helper function to identify which part of the pass state has changed
static inline bool has_mutated(uint64_t state, uint64_t old_state, uint64_t mask)
{
	return ((state^old_state)&mask) > 0;
}

static void handle_state(uint64_t state_flags)
{
	// * If pass state has changed, decode it, find which parts have changed and update device state
	if(state_flags != s_storage.state_cache_)
	{
		RenderState state;
		state.decode(state_flags);

		if(has_mutated(state_flags, s_storage.state_cache_, k_framebuffer_mask))
		{
			if(state.render_target == s_storage.default_framebuffer_)
			{
				Gfx::device->bind_default_framebuffer();
				Gfx::device->viewport(0, 0, s_storage.host_window_size_.x, s_storage.host_window_size_.y);
			}
			else
				s_storage.framebuffers[state.render_target.index]->bind();

			s_storage.current_framebuffer_ = state.render_target;

			// Only clear on render target switch, if clear flags are set
			if(state.rasterizer_state.clear_flags != ClearFlags::CLEAR_NONE)
				Gfx::device->clear(state.rasterizer_state.clear_flags);
		}

		if(has_mutated(state_flags, s_storage.state_cache_, k_cull_mode_mask))
			Gfx::device->set_cull_mode(state.rasterizer_state.cull_mode);
		
		if(has_mutated(state_flags, s_storage.state_cache_, k_transp_mask))
		{
			switch(state.blend_state)
			{
				case BlendState::Alpha: Gfx::device->set_std_blending(); break;
				case BlendState::Light: Gfx::device->set_light_blending(); break;
				default:                Gfx::device->disable_blending(); break;
			}
		}

		if(has_mutated(state_flags, s_storage.state_cache_, k_stencil_test_mask))
		{
			Gfx::device->set_stencil_test_enabled(state.depth_stencil_state.stencil_test_enabled);
			if(state.depth_stencil_state.stencil_test_enabled)
			{
				Gfx::device->set_stencil_func(state.depth_stencil_state.stencil_func);
				Gfx::device->set_stencil_operator(state.depth_stencil_state.stencil_operator);
			}
		}

		if(has_mutated(state_flags, s_storage.state_cache_, k_depth_test_mask))
		{
			Gfx::device->set_depth_test_enabled(state.depth_stencil_state.depth_test_enabled);
			if(state.depth_stencil_state.depth_test_enabled)
				Gfx::device->set_depth_func(state.depth_stencil_state.depth_func);
		}

		if(has_mutated(state_flags, s_storage.state_cache_, k_depth_lock_mask))
			Gfx::device->set_depth_lock(state.depth_stencil_state.depth_lock);

		if(has_mutated(state_flags, s_storage.state_cache_, k_stencil_lock_mask))
			Gfx::device->set_stencil_lock(state.depth_stencil_state.stencil_lock);

		s_storage.state_cache_ = state_flags;
	}
}

namespace draw_dispatch
{

void draw(memory::LinearBuffer<>& buf)
{
    W_PROFILE_RENDER_FUNCTION()

    DrawCall::DrawCallType type;
    DrawCall::Data data;
	buf.read(&type);
	buf.read(&data);

	handle_state(data.state_flags);

	// * Detect if a new shader needs to be used, update and bind shader resources
	static uint16_t last_shader_index = k_invalid_handle;
	static uint16_t last_texture_index[k_max_texture_slots];
	static uint16_t last_cubemap_index[k_max_cubemap_slots];
	auto& shader = *s_storage.shaders[data.shader.index];
	if(data.shader.index != last_shader_index)
	{
		shader.bind();
		last_shader_index = data.shader.index;
		std::fill(last_texture_index, last_texture_index+k_max_texture_slots, k_invalid_handle);
		std::fill(last_cubemap_index, last_cubemap_index+k_max_cubemap_slots, k_invalid_handle);
	}

	uint8_t texture_count;
	buf.read(&texture_count);
	for(uint8_t ii=0; ii<texture_count; ++ii)
	{
		TextureHandle hnd;
		buf.read(&hnd);

		// Avoid texture switching if not necessary
		if(hnd.index != last_texture_index[ii])
		{
			auto& texture = *s_storage.textures[hnd.index];
			shader.attach_texture_2D(texture, ii);
			last_texture_index[ii] = hnd.index;
		}
	}

	uint8_t cubemap_count;
	buf.read(&cubemap_count);
	for(uint8_t ii=0; ii<cubemap_count; ++ii)
	{
		CubemapHandle hnd;
		buf.read(&hnd);

		// Avoid texture switching if not necessary
		if(hnd.index != last_cubemap_index[ii])
		{
			auto& cubemap = *s_storage.cubemaps[hnd.index];
			shader.attach_cubemap(cubemap, ii);
			last_cubemap_index[ii] = hnd.index;
		}
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
			uint32_t instance_count;
			buf.read(&instance_count);
			// ASSUME SSBO is attached to shader, so it is already bound at this stage
			Gfx::device->draw_indexed_instanced(va, instance_count, data.count, data.offset);
			break;
		}
		default:
			W_ASSERT(false, "Specified draw call type is unsupported at the moment.");
			break;
	}
}

void clear(memory::LinearBuffer<>& buf)
{
	FramebufferHandle target;
	uint32_t flags;
	uint32_t clear_color;
	buf.read(&target);
	buf.read(&flags);
	buf.read(&clear_color);

	glm::vec4 color = color::unpack(clear_color);

    Gfx::device->set_clear_color(color.r, color.g, color.b, color.a);
	if(target == s_storage.default_framebuffer_ && flags != ClearFlags::CLEAR_NONE)
	{
		Gfx::device->bind_default_framebuffer();
		Gfx::device->viewport(0, 0, s_storage.host_window_size_.x, s_storage.host_window_size_.y);
		Gfx::device->clear(int(flags));
	}
	else
	{
		auto& fb = *s_storage.framebuffers[target.index];
		fb.bind();
		Gfx::device->viewport(0, 0, float(fb.get_width()), float(fb.get_height()));
		Gfx::device->clear(int(flags));
	}

	if(s_storage.current_framebuffer_ != target)
	{
		// Rebind current framebuffer
		if(s_storage.current_framebuffer_ == s_storage.default_framebuffer_)
		{
			Gfx::device->bind_default_framebuffer();
			Gfx::device->viewport(0, 0, s_storage.host_window_size_.x, s_storage.host_window_size_.y);
		}
		else
		{
			auto& fb = *s_storage.framebuffers[s_storage.current_framebuffer_.index];
			fb.bind();
			Gfx::device->viewport(0, 0, float(fb.get_width()), float(fb.get_height()));
		}
	}
    Gfx::device->set_clear_color(0.f,0.f,0.f,0.f);
}

void blit_depth(memory::LinearBuffer<>& buf)
{
	FramebufferHandle source;
	FramebufferHandle target;
	buf.read(&source);
	buf.read(&target);

	s_storage.framebuffers[target.index]->blit_depth(*s_storage.framebuffers[source.index]);
}

void update_shader_storage_buffer(memory::LinearBuffer<>& buf)
{
	ShaderStorageBufferHandle ssbo_handle;
	uint32_t size;
	void* data;

	buf.read(&ssbo_handle);
	buf.read(&size);
	buf.read(&data);

	s_storage.shader_storage_buffers[ssbo_handle.index]->stream(data, size, 0);
/*
	auto& ssbo = *s_storage.shader_storage_buffers[ssbo_handle.index];
	if(!ssbo.has_persistent_mapping())
		ssbo.stream(data, size, 0);
	else
		ssbo.map_persistent(data, size, 0);
*/
}

void update_uniform_buffer(memory::LinearBuffer<>& buf)
{
	UniformBufferHandle ubo_handle;
	uint32_t size;
	void* data;

	buf.read(&ubo_handle);
	buf.read(&size);
	buf.read(&data);

	auto& ubo = *s_storage.uniform_buffers[ubo_handle.index];
	ubo.stream(data, size ? size : ubo.get_size(), 0);
}

} // namespace draw_dispatch

static backend_dispatch_func_t draw_backend_dispatch[std::size_t(RenderCommand::Count)] =
{
	&draw_dispatch::draw,
	&draw_dispatch::clear,
	&draw_dispatch::blit_depth,
	&draw_dispatch::update_shader_storage_buffer,
	&draw_dispatch::update_uniform_buffer,
};



void RenderQueue::flush()
{
    W_PROFILE_RENDER_FUNCTION()
    // Set clear color
    Gfx::device->set_clear_color(clear_color_.r, clear_color_.g, clear_color_.b, clear_color_.a);

    // Execute all draw commands in command buffer
	for(size_t ii=0; ii<command_buffer_.count; ++ii)
	{
		auto&& [key,cmd] = command_buffer_.entries[ii];

		// k_skip is the max key value possible, so if we arrive here, simply break out of the loop
		if(key == SortKey::k_skip) break;

#if W_RC_PROFILE_DRAW_CALLS
		if(s_storage.draw_call_data.tracking)
			s_storage.draw_call_data.on_dispatch(key);
#endif
		command_buffer_.storage.seek(cmd);

		uint16_t type;
		command_buffer_.storage.read(&type);

		// If type is a draw call, unroll and dispatch dependencies first
		if(type == uint16_t(DrawCommand::Draw))
		{
		    uint8_t dependency_count;
		    static void* deps[k_max_draw_call_dependencies];
			command_buffer_.storage.read(&dependency_count);
		    for(uint8_t jj=0; jj<dependency_count; ++jj)
				command_buffer_.storage.read(&deps[jj]);

			void* ret = command_buffer_.storage.head();
		    for(uint8_t jj=0; jj<dependency_count; ++jj)
			{
				command_buffer_.storage.seek(deps[jj]);
				uint16_t dep_type;
				command_buffer_.storage.read(&dep_type);
				(*draw_backend_dispatch[dep_type])(command_buffer_.storage);
			}
			command_buffer_.storage.seek(ret);
		}

		(*draw_backend_dispatch[type])(command_buffer_.storage);
	}
}

static void flush_command_buffer(RenderCommandBuffer& cmdbuf)
{
    W_PROFILE_RENDER_FUNCTION()

    // Dispatch render commands in specified command buffer
	for(size_t ii=0; ii<cmdbuf.count; ++ii)
	{
		auto&& [key,cmd] = cmdbuf.entries[ii];
		cmdbuf.storage.seek(cmd);
		uint16_t type;
		cmdbuf.storage.read(&type);
		(*render_backend_dispatch[type])(cmdbuf.storage);
	}
	cmdbuf.reset();
}

static void sort_commands()
{
    W_PROFILE_RENDER_FUNCTION()
	// Keys stored separately from commands to avoid touching data too
	// much during sort calls
    std::sort(std::begin(s_storage.pre_buffer_.entries), std::begin(s_storage.pre_buffer_.entries) + s_storage.pre_buffer_.count, 
        [&](const RenderCommandBuffer::Entry& item1, const RenderCommandBuffer::Entry& item2)
        {
        	return item1.first < item2.first;
        });
    std::sort(std::begin(s_storage.post_buffer_.entries), std::begin(s_storage.post_buffer_.entries) + s_storage.post_buffer_.count, 
        [&](const RenderCommandBuffer::Entry& item1, const RenderCommandBuffer::Entry& item2)
        {
        	return item1.first < item2.first;
        });
}

void Renderer::flush()
{
    W_PROFILE_RENDER_FUNCTION()
    static nanoClock flush_clock;
	if(s_storage.profiling_enabled)
	{
		s_storage.query_timer->start();
    	flush_clock.restart();
	}

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

#if W_RC_PROFILE_DRAW_CALLS
	if(s_storage.draw_call_data.tracking)
		s_storage.draw_call_data.export_json();
#endif

	if(s_storage.profiling_enabled)
	{
		auto GPU_render_duration = s_storage.query_timer->stop();
        auto CPU_flush_duration  = flush_clock.get_elapsed_time();
		s_storage.stats[FRONT].GPU_render_time = float(std::chrono::duration_cast<std::chrono::microseconds>(GPU_render_duration).count());
		s_storage.stats[FRONT].CPU_flush_time  = float(std::chrono::duration_cast<std::chrono::microseconds>(CPU_flush_duration).count());
	}

	std::swap(FRONT, BACK);
	s_storage.stats[FRONT].draw_call_count = 0;
}

} // namespace erwin