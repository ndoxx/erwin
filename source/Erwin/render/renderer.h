#pragma once

#include <cstdint>
#include <cmath>
#include <utility>
#include <functional>
#include <future>
#include <filesystem>

#include "memory/memory.hpp"
#include "render/render_state.h"
#include "render/buffer_layout.h"
#include "render/framebuffer_layout.h"
#include "render/framebuffer_pool.h"
#include "render/texture_common.h"
#include "render/handles.h"
#include "render/sort_key.h"
#include "render/commands.h"

namespace fs = std::filesystem;

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
	// Get a handle to the cubemap attachment of a given framebuffer
	static CubemapHandle 	  get_framebuffer_cubemap(FramebufferHandle handle);
	// Get the debug name of a specified color or depth attachment of a given framebuffer
	static hash_t 			  get_framebuffer_texture_name(FramebufferHandle handle, uint32_t index);
	// Get the number of attachments in a given framebuffer
	static uint32_t 		  get_framebuffer_texture_count(FramebufferHandle handle);
	// Create a layout for a vertex buffer. Creation is immediate as it does not imply render API stuff,
	// however, layout destruction need be deferred and is handled by a command.
	static VertexBufferLayoutHandle create_vertex_buffer_layout(const std::vector<BufferLayoutElement>& elements);
	// Get a buffer layout from its handle
	static const BufferLayout& get_vertex_buffer_layout(VertexBufferLayoutHandle handle);
	// Check compatibility between a buffer layout and a shader's attributes layout
	//static bool is_compatible(VertexBufferLayoutHandle layout, ShaderHandle shader);
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
	static uint32_t update_shader_storage_buffer(ShaderStorageBufferHandle handle, const void* data, uint32_t size, DataOwnership copy);
	// Update an UBO's data
	static uint32_t update_uniform_buffer(UniformBufferHandle handle, const void* data, uint32_t size, DataOwnership copy);

	// Force renderer to dispatch all render/draw commands
	static void flush();

	// * The following functions will initialize a render command and push it to the appropriate buffer 
	// PRE-BUFFER -> executed before draw commands
	static IndexBufferHandle         create_index_buffer(const uint32_t* index_data, uint32_t count, DrawPrimitive primitive, UsagePattern mode = UsagePattern::Static);
	static VertexBufferHandle        create_vertex_buffer(VertexBufferLayoutHandle layout, const float* vertex_data, uint32_t count, UsagePattern mode = UsagePattern::Static);
	static VertexArrayHandle         create_vertex_array(VertexBufferHandle vb, IndexBufferHandle ib);
	static VertexArrayHandle         create_vertex_array(const std::vector<VertexBufferHandle>& vbs, IndexBufferHandle ib);
	static UniformBufferHandle       create_uniform_buffer(const std::string& name, const void* data, uint32_t size, UsagePattern mode = UsagePattern::Dynamic);
	static ShaderStorageBufferHandle create_shader_storage_buffer(const std::string& name, const void* data, uint32_t size, UsagePattern mode = UsagePattern::Dynamic);
	static ShaderHandle 			 create_shader(const fs::path& filepath, const std::string& name);
	static TextureHandle 			 create_texture_2D(const Texture2DDescriptor& desc);
	static CubemapHandle 			 create_cubemap(const CubemapDescriptor& desc);
	static FramebufferHandle 		 create_framebuffer(uint32_t width, uint32_t height, uint8_t flags, const FramebufferLayout& layout);
	static void 					 update_index_buffer(IndexBufferHandle handle, const uint32_t* data, uint32_t count);
	static void 					 update_vertex_buffer(VertexBufferHandle handle, const void* data, uint32_t size);
	static void 					 update_uniform_buffer(UniformBufferHandle handle, const void* data, uint32_t size);
	static void 					 update_shader_storage_buffer(ShaderStorageBufferHandle handle, const void* data, uint32_t size);
	static void 					 shader_attach_uniform_buffer(ShaderHandle shader, UniformBufferHandle ubo);
	static void 					 shader_attach_storage_buffer(ShaderHandle shader, ShaderStorageBufferHandle ssbo);
	static void 					 update_framebuffer(FramebufferHandle fb, uint32_t width, uint32_t height);
	static void 					 clear_framebuffers();
	static void						 set_host_window_size(uint32_t width, uint32_t height);
	// POST-BUFFER -> executed after draw commands
	static std::future<PixelData>    get_pixel_data(TextureHandle handle);
	static void 					 generate_mipmaps(CubemapHandle cubemap);
	static void 					 framebuffer_screenshot(FramebufferHandle fb, const fs::path& filepath);
	static void 					 destroy(IndexBufferHandle handle);
	static void 					 destroy(VertexBufferLayoutHandle handle);
	static void 					 destroy(VertexBufferHandle handle);
	static void 					 destroy(VertexArrayHandle handle);
	static void 					 destroy(UniformBufferHandle handle);
	static void 					 destroy(ShaderStorageBufferHandle handle);
	static void 					 destroy(ShaderHandle handle);
	static void 					 destroy(TextureHandle handle);
	static void 					 destroy(CubemapHandle handle);
	static void 					 destroy(FramebufferHandle handle, bool detach_textures=false);

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

} // namespace erwin