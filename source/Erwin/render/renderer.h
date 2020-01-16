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
		float render_time = 0.f;
	};

	// * The following functions have immediate effect
	// Require a view id for a pass
	static uint8_t           next_view_id();
	// Get the renderer memory arena, for per-frame data allocation outside of the renderer 
	static AuxArena& 		 get_arena();
	// Get a handle to the default framebuffer (screen)
	static FramebufferHandle default_render_target();
	// Get a handle to a specified color or depth attachment of a given framebuffer
	static TextureHandle 	 get_framebuffer_texture(FramebufferHandle handle, uint32_t index);
	// Get the number of attachments in a given framebuffer
	static uint32_t 		 get_framebuffer_texture_count(FramebufferHandle handle);
	// Create a layout for a vertex buffer. Creation is immediate as it does not imply render API stuff,
	// however, layout destruction need be deferred and is handled by a command.
	static VertexBufferLayoutHandle create_vertex_buffer_layout(const std::vector<BufferLayoutElement>& elements);
	// Get a buffer layout from its handle
	static const BufferLayout& get_vertex_buffer_layout(VertexBufferLayoutHandle handle);

	// * Draw call queue management and submission
	// Send a draw call to the queue
	static void submit(const DrawCall& dc);
	// Blit depth buffer / texture from source to target
	static void blit_depth(FramebufferHandle source, FramebufferHandle target, uint64_t key);

	// Force renderer to dispatch all commands in command buffers and draw calls in the render queue
	static void flush();
	// Set a callback function that will be executed after flush()
	static void set_end_frame_callback(std::function<void(void)> callback);

	// * The following functions will initialize a render command and push it to the appropriate buffer 
	// PRE-BUFFER -> executed before draw commands
	static IndexBufferHandle         create_index_buffer(const uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode = DrawMode::Static);
	static VertexBufferHandle        create_vertex_buffer(VertexBufferLayoutHandle layout, const float* vertex_data, uint32_t count, DrawMode mode = DrawMode::Static);
	static VertexArrayHandle         create_vertex_array(VertexBufferHandle vb, IndexBufferHandle ib);
	static UniformBufferHandle       create_uniform_buffer(const std::string& name, void* data, uint32_t size, DrawMode mode = DrawMode::Dynamic);
	static ShaderStorageBufferHandle create_shader_storage_buffer(const std::string& name, void* data, uint32_t size, DrawMode mode = DrawMode::Dynamic);
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
	static void* get_native_texture_handle(TextureHandle handle);
	static void set_profiling_enabled(bool value=true);
	static const Statistics& get_stats();
#endif
#if W_RC_PROFILE_DRAW_CALLS
	// Track draw calls for this frame
	static void track_draw_calls(const fs::path& json_path);
#endif

private:
	friend class Application;
	friend class RenderQueue;

	static void init(memory::HeapArea& area);
	static void shutdown();
};

// Helper struct to simplify the generation of a sorting key associated to a draw call
struct SortKey
{
	// Policy for key sorting
	enum class Order: uint8_t
	{
		ByShader,			// Keys are sorted by shader in priority, then by depth (for compute shaders)
		ByDepthDescending,	// Keys are sorted by increasing clip depth first, then by shader
		ByDepthAscending,	// Keys are sorted by decreasing clip depth first, then by shader
		Sequential 			// Keys are sorted by a 32-bits sequence number
	};

	// Encode key structure into a 64 bits number (the actual sorting key)
	uint64_t encode() const;

	uint16_t view = 0;     // [view id | framebuffer id] for depth order, just view id for sequential order
	uint8_t shader = 0;    // shader id to allow grouping by shader
	uint32_t depth = 0;    // depth mantissa
	uint32_t sequence = 0; // for commands to be dispatched sequentially
	bool blending = false; // affects the draw_type bit
	SortKey::Order order;  // impacts how this key will be encoded
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

	enum DataOwnership: uint8_t
	{
		ForwardData = 0, // Do not copy data, forward pointer as is
		CopyData = 1     // Copy data to renderer memory
	};

	#pragma pack(push,1)
	struct Data
	{
		uint64_t state_flags;

		ShaderHandle shader;
		VertexArrayHandle VAO;
		TextureHandle textures[k_max_texture_slots];
		UniformBufferHandle UBOs[k_max_UBO_slots];
		void* UBOs_data[k_max_UBO_slots];

		uint32_t count;
		uint32_t offset;
	} data;
	struct InstanceData
	{
		void* SSBO_data;
		uint32_t SSBO_size;
		uint32_t instance_count;
		ShaderStorageBufferHandle SSBO;
	} instance_data;
	#pragma pack(pop)

	SortKey key;
	DrawCallType type;

	DrawCall(DrawCallType dc_type, uint8_t layer_id, uint64_t state, ShaderHandle shader, VertexArrayHandle VAO, uint32_t count=0, uint32_t offset=0)
	{
		W_ASSERT(shader.is_valid(), "Invalid ShaderHandle!");
		W_ASSERT(VAO.is_valid(), "Invalid VertexArrayHandle!");

		type             = dc_type;
		data.state_flags = state;
		data.shader      = shader;
		data.VAO         = VAO;
		data.count       = count;
		data.offset      = offset;
		instance_data.SSBO_data = nullptr;
		instance_data.SSBO_size = 0;

		// Setup sorting key
		key.view   = (uint16_t(layer_id)<<8);
		key.shader = data.shader.index; // NOTE(ndoxx): Overflow when shader index is greater than 255
	}

	// Set instance data array containing all information necessary to render instance_count instances of the same geometry
	// Only available for instanced draw calls
	inline void set_SSBO(ShaderStorageBufferHandle ssbo, void* SSBO_data, uint32_t size, uint32_t inst_count, DataOwnership copy)
	{
		W_ASSERT(type == DrawCall::IndexedInstanced || type == DrawCall::ArrayInstanced, "Cannot set instance data for non-instanced draw call.");

		instance_data.SSBO_data = SSBO_data;
		instance_data.SSBO = ssbo;
		instance_data.SSBO_size = size;
		instance_data.instance_count = inst_count;

		if(SSBO_data && copy)
		{
			instance_data.SSBO_data = W_NEW_ARRAY_DYNAMIC(uint8_t, size, Renderer::get_arena());
			memcpy(instance_data.SSBO_data, SSBO_data, size);
		}
	}

	// Setup a UBO configuration for this specific draw call
	inline void set_UBO(UniformBufferHandle ubo, void* UBO_data, uint32_t size, DataOwnership copy, uint32_t slot=0)
	{
		W_ASSERT_FMT(slot<k_max_UBO_slots, "UBO slot out of bounds: %u", slot);
		data.UBOs_data[slot] = UBO_data;
		data.UBOs[slot] = ubo;

		if(UBO_data && copy)
		{
			data.UBOs_data[slot] = W_NEW_ARRAY_DYNAMIC(uint8_t, size, Renderer::get_arena());
			memcpy(data.UBOs_data[slot], UBO_data, size);
		}
	}

	// Set a texture at a given slot
	inline void set_texture(TextureHandle tex, uint32_t slot=0)
	{
		W_ASSERT_FMT(tex.is_valid(), "Invalid TextureHandle of index: %hu", tex.index);
		W_ASSERT_FMT(slot<k_max_texture_slots, "Texture slot out of bounds: %u", slot);
		data.textures[slot] = tex;
	}

	// Compute the sorting key for depth ascending/descending policies
	inline void set_key_depth(float depth)
	{
		W_ASSERT(data.shader.index<256, "Shader index out of bounds in shader sorting key section.");
		key.blending = RenderState::is_transparent(data.state_flags);
		key.view |= uint8_t((data.state_flags & k_framebuffer_mask) >> k_framebuffer_shift);
		key.depth = *((uint32_t*)(&depth)); // TODO: Normalize depth and extract 24b mantissa
		key.order = RenderState::is_transparent(data.state_flags) ? SortKey::Order::ByDepthAscending : SortKey::Order::ByDepthDescending;
	}

	// Compute the sorting key for the sequential policy
	inline void set_key_sequence(uint32_t sequence)
	{
		W_ASSERT(data.shader.index<256, "Shader index out of bounds in shader sorting key section.");
		key.blending = false;
		key.sequence = sequence;
		key.order = SortKey::Order::Sequential;
	}
};


} // namespace erwin