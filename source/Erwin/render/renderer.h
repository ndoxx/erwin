#pragma once

#include <cstdint>
#include <cmath>
#include <utility>
#include <functional>

#include "filesystem/filesystem.h"
#include "memory/memory.hpp"
#include "render/render_state.h"
#include "render/buffer_layout.h"
#include "render/framebuffer_layout.h"
#include "render/texture_common.h"
#include "render/handles.h"
#include "render/framebuffer_pool.h"
#include "render/renderer_config.h"

namespace erwin
{

enum class DataOwnership: uint8_t
{
	Forward = 0, // Do not copy data, forward pointer as is
	Copy = 1     // Copy data to renderer memory
};

struct DrawCall;
#if W_RC_PROFILE_DRAW_CALLS
struct FrameDrawCallData;
#endif
class Renderer
{
public:
	typedef memory::MemoryArena<memory::LinearAllocator, 
			    				memory::policy::SingleThread, 
			    				memory::policy::NoBoundsChecking,
			    				memory::policy::NoMemoryTagging,
			    				memory::policy::NoMemoryTracking> AuxArena;
			    				
	struct Statistics
	{
		float GPU_render_time = 0.f;
		float CPU_flush_time = 0.f;
		uint32_t draw_call_count = 0;
	};

	// * The following functions have immediate effect
	// Require a layer id for a pass
	static uint8_t            next_layer_id();
	// Get the renderer memory arena, for per-frame data allocation outside of the renderer 
	static AuxArena& 		  get_arena();
	// Get a handle to the default framebuffer (screen)
	static FramebufferHandle  default_render_target();
	// Get a handle to a specified color or depth attachment of a given framebuffer
	static TextureHandle 	  get_framebuffer_texture(FramebufferHandle handle, uint32_t index);
	// Get the debug name of a specified color or depth attachment of a given framebuffer
	static hash_t 			  get_framebuffer_texture_name(FramebufferHandle handle, uint32_t index);
	// Get the number of attachments in a given framebuffer
	static uint32_t 		  get_framebuffer_texture_count(FramebufferHandle handle);
	// Create a layout for a vertex buffer. Creation is immediate as it does not imply render API stuff,
	// however, layout destruction need be deferred and is handled by a command.
	static VertexBufferLayoutHandle create_vertex_buffer_layout(const std::vector<BufferLayoutElement>& elements);
	// Get a buffer layout from its handle
	static const BufferLayout& get_vertex_buffer_layout(VertexBufferLayoutHandle handle);
	// Get native handle to a texture
	static void* get_native_texture_handle(TextureHandle handle);

	// * Draw command queue management and submission
	// Send a draw call to the queue
	static void submit(uint64_t key, const DrawCall& dc);
	// Clear render target
	static void clear(uint64_t key, FramebufferHandle target, uint32_t flags, const glm::vec4& clear_color={0.f,0.f,0.f,0.f});
	// Blit depth buffer / texture from source to target
	static void blit_depth(uint64_t key, FramebufferHandle source, FramebufferHandle target);
	// * Draw call dependencies
	// Update an SSBO's data
	static uint32_t update_shader_storage_buffer(ShaderStorageBufferHandle handle, void* data, uint32_t size, DataOwnership copy);
	// Update an UBO's data
	static uint32_t update_uniform_buffer(UniformBufferHandle handle, void* data, uint32_t size, DataOwnership copy);

	// Force renderer to dispatch all render/draw commands
	static void flush();
	// Set a callback function that will be executed after flush()
	static void set_end_frame_callback(std::function<void(void)> callback);

	// * The following functions will initialize a render command and push it to the appropriate buffer 
	// PRE-BUFFER -> executed before draw commands
	static IndexBufferHandle         create_index_buffer(const uint32_t* index_data, uint32_t count, DrawPrimitive primitive, UsagePattern mode = UsagePattern::Static);
	static VertexBufferHandle        create_vertex_buffer(VertexBufferLayoutHandle layout, const float* vertex_data, uint32_t count, UsagePattern mode = UsagePattern::Static);
	static VertexArrayHandle         create_vertex_array(VertexBufferHandle vb, IndexBufferHandle ib);
	static VertexArrayHandle         create_vertex_array(const std::vector<VertexBufferHandle>& vbs, IndexBufferHandle ib);
	static UniformBufferHandle       create_uniform_buffer(const std::string& name, void* data, uint32_t size, UsagePattern mode = UsagePattern::Dynamic);
	static ShaderStorageBufferHandle create_shader_storage_buffer(const std::string& name, void* data, uint32_t size, UsagePattern mode = UsagePattern::Dynamic);
	static ShaderHandle 			 create_shader(const fs::path& filepath, const std::string& name);
	static TextureHandle 			 create_texture_2D(const Texture2DDescriptor& desc);
	static FramebufferHandle 		 create_framebuffer(uint32_t width, uint32_t height, bool depth, bool stencil, const FramebufferLayout& layout);
	static void 					 update_index_buffer(IndexBufferHandle handle, uint32_t* data, uint32_t count);
	static void 					 update_vertex_buffer(VertexBufferHandle handle, void* data, uint32_t size);
	static void 					 update_uniform_buffer(UniformBufferHandle handle, void* data, uint32_t size);
	static void 					 update_shader_storage_buffer(ShaderStorageBufferHandle handle, void* data, uint32_t size);
	static void 					 shader_attach_uniform_buffer(ShaderHandle shader, UniformBufferHandle ubo);
	static void 					 shader_attach_storage_buffer(ShaderHandle shader, ShaderStorageBufferHandle ssbo);
	static void 					 update_framebuffer(FramebufferHandle fb, uint32_t width, uint32_t height);
	static void 					 clear_framebuffers();
	static void						 set_host_window_size(uint32_t width, uint32_t height);
	// POST-BUFFER -> executed after draw commands
	static void 					 framebuffer_screenshot(FramebufferHandle fb, const fs::path& filepath);
	static void 					 destroy(IndexBufferHandle handle);
	static void 					 destroy(VertexBufferLayoutHandle handle);
	static void 					 destroy(VertexBufferHandle handle);
	static void 					 destroy(VertexArrayHandle handle);
	static void 					 destroy(UniformBufferHandle handle);
	static void 					 destroy(ShaderStorageBufferHandle handle);
	static void 					 destroy(ShaderHandle handle);
	static void 					 destroy(TextureHandle handle);
	static void 					 destroy(FramebufferHandle handle);

#ifdef W_DEBUG
	static void set_profiling_enabled(bool value=true);
	static const Statistics& get_stats();
#endif
	static void track_draw_calls(const fs::path& json_path);

private:
	friend class Application;
	friend class RenderQueue;

	static void init(memory::HeapArea& area);
	static void shutdown();
};

// Helper struct to simplify the generation of a sorting key associated to a draw command
struct SortKey
{
	// Policy for key sorting
	enum class Order: uint8_t
	{
		ByShader,			// Keys are sorted by shader in priority, then by depth
		ByDepthDescending,	// Keys are sorted by increasing clip depth first, then by shader
		ByDepthAscending,	// Keys are sorted by decreasing clip depth first, then by shader
		Sequential 			// Keys are sorted by a 32-bits sequence number
	};

	static constexpr uint64_t k_skip = std::numeric_limits<uint64_t>::max();

	// Encode key structure into a 64 bits number (the actual sorting key)
	uint64_t encode() const;

	inline void set_depth(float _depth, uint8_t layer_id, uint64_t state_flags, ShaderHandle shader_handle, uint8_t _sub_sequence=0)
	{
		W_ASSERT(shader_handle.index<256, "Shader index out of bounds in shader sorting key section.");
		view         = (uint16_t(layer_id)<<8);
		view        |= uint8_t((state_flags & k_framebuffer_mask) >> k_framebuffer_shift);
		shader       = shader_handle.index;
		sub_sequence = _sub_sequence;
		depth        = uint32_t(glm::clamp(std::fabs(_depth), 0.f, 1.f) * 0x00ffffff);
		blending     = RenderState::is_transparent(state_flags);
		order        = blending ? SortKey::Order::ByDepthAscending : SortKey::Order::ByDepthDescending;
	}

	inline void set_sequence(uint32_t _sequence, uint8_t layer_id, ShaderHandle shader_handle, uint8_t _sub_sequence=0)
	{
		W_ASSERT(shader_handle.index<256, "Shader index out of bounds in shader sorting key section.");
		view         = (uint16_t(layer_id)<<8);
		shader       = shader_handle.index;
		sub_sequence = _sub_sequence;
		sequence     = _sequence;
		blending     = false;
		order        = SortKey::Order::Sequential;
	}

	uint16_t view = 0;        // [layer id | framebuffer id] for depth order, just layer id for sequential order
	uint8_t shader = 0;       // shader id to allow grouping by shader
	uint8_t sub_sequence = 0; // allows draw command chaining even in depth mode
	uint32_t depth = 0;       // 24 bits clamped absolute normalized depth
	uint32_t sequence = 0;    // for commands to be dispatched sequentially
	bool blending = false;    // affects the draw_type bits
	SortKey::Order order;     // impacts how this key will be encoded
};

// All the state needed by the renderer to perform a platform draw call
struct DrawCall
{
	enum DrawCallType: uint8_t
	{
		Indexed,
		Array,
		IndexedInstanced,
		ArrayInstanced,

		Count
	};

	#pragma pack(push,1)
	struct Data
	{
		uint64_t state_flags;
		uint32_t count;
		uint32_t offset;

		ShaderHandle shader;
		VertexArrayHandle VAO;
		TextureHandle textures[k_max_texture_slots];
	} data;
	#pragma pack(pop)
	uint32_t instance_count;
	uint32_t dependencies[k_max_draw_call_dependencies];
	DrawCallType type;
	uint8_t dependency_count;

	DrawCall(DrawCallType dc_type, uint64_t state, ShaderHandle shader, VertexArrayHandle VAO, uint32_t count=0, uint32_t offset=0)
	{
		W_ASSERT(shader.is_valid(), "Invalid ShaderHandle!");
		W_ASSERT(VAO.is_valid(), "Invalid VertexArrayHandle!");

		dependency_count = 0;
		type             = dc_type;
		data.state_flags = state;
		data.shader      = shader;
		data.VAO         = VAO;
		data.count       = count;
		data.offset      = offset;
	}

	inline void add_dependency(uint32_t token)
	{
		W_ASSERT(dependency_count<k_max_draw_call_dependencies-1, "Exceeding draw call max dependency count.");
		dependencies[dependency_count++] = token;
	}

	inline void set_instance_count(uint32_t value)
	{
		instance_count = value;
	}

	// Set a texture at a given slot
	inline void set_texture(TextureHandle tex, uint32_t slot=0)
	{
		W_ASSERT_FMT(tex.is_valid(), "Invalid TextureHandle of index: %hu", tex.index);
		W_ASSERT_FMT(slot<k_max_texture_slots, "Texture slot out of bounds: %u", slot);
		data.textures[slot] = tex;
	}
};


} // namespace erwin