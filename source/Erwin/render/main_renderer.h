#pragma once

#include <cstdint>
#include <cmath>
#include <utility>

#include "filesystem/filesystem.h"
#include "render/render_state.h"
#include "render/buffer_layout.h"
#include "render/framebuffer_layout.h"
#include "memory/arena.h"

#include "render/texture.h" // TMP: for Texture2DDescriptor
#include "render/handles.h"

#include "render/framebuffer_pool.h"

namespace erwin
{

struct SortKey
{
	enum class Order: uint8_t
	{
		ByShader,
		ByDepthDescending,
		ByDepthAscending,
		Sequential
	};

	uint64_t encode(SortKey::Order type) const;
	void decode(uint64_t key);

						      // -- meaning --
	uint16_t view = 0;        // layer / viewport id
	uint8_t shader = 0;       // could be "material ID" when I have a material system
	uint32_t depth = 0;       // depth mantissa
	uint32_t sequence = 0;    // for commands to be dispatched sequentially
};

struct MainRendererStats
{
	float render_time = 0.f;
};

class RenderQueue;
struct DrawCall;
class MainRenderer
{
public:
	typedef memory::MemoryArena<memory::LinearAllocator, 
			    				memory::policy::SingleThread, 
			    				memory::policy::NoBoundsChecking,
			    				memory::policy::NoMemoryTagging,
			    				memory::policy::NoMemoryTracking> AuxArena;
			    				
	// * The following functions have immediate effect
	// Get the renderer memory arena, for per-frame data allocation outside of the renderer 
	static AuxArena& 		 get_arena();
	// Get a handle to the default framebuffer (screen)
	static FramebufferHandle default_render_target();
	// Get a handle to a specified color or depth attachment of a given framebuffer
	static TextureHandle 	 get_framebuffer_texture(FramebufferHandle handle, uint32_t index);
	// Get the number of attachments in a given framebuffer
	static uint32_t 		 get_framebuffer_texture_count(FramebufferHandle handle);

	// * Draw call queue management and submission
	// Create a render queue, specifying a name whose hash is used to retrieve it later on, and a draw order policy
	static RenderQueue& 	 create_queue(const std::string& name, SortKey::Order order);
	// Get a render queue by name
	static RenderQueue& 	 get_queue(hash_t name);
	// Send a draw call to a particular queue
	static inline void 		 submit(hash_t queue, const DrawCall& dc);
	// Force renderer to dispatch all commands in command buffers and render queues
	static void 			 flush();

	// * The following functions will initialize a render command and push it to the appropriate buffer 
	// PRE-BUFFER -> executed before draw commands
	static IndexBufferHandle         create_index_buffer(uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode = DrawMode::Static);
	static VertexBufferLayoutHandle  create_vertex_buffer_layout(const std::initializer_list<BufferLayoutElement>& elements);
	static VertexBufferHandle        create_vertex_buffer(VertexBufferLayoutHandle layout, float* vertex_data, uint32_t count, DrawMode mode = DrawMode::Static);
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
	static const MainRendererStats& get_stats();
#endif

private:
	friend class Application;

	static void init(memory::HeapArea& area);
	static void shutdown();
};

constexpr std::size_t k_max_render_commands = 10000;

struct CommandBuffer
{
	typedef std::pair<uint64_t,void*> Entry;

	CommandBuffer(std::pair<void*,void*> ptr_range):
	count(0),
	storage(ptr_range)
	{

	}

	inline void reset()
	{
		storage.reset();
		count = 0;
	}

	std::size_t count;
	memory::LinearBuffer<> storage;
	Entry entries[k_max_render_commands];
};

struct DrawCall;
class RenderQueue
{
public:
	friend class MainRenderer;

	RenderQueue(SortKey::Order order, memory::HeapArea& area);
	~RenderQueue();

	// * These functions change the queue state persistently
	// Set clear color for this queue
	inline void set_clear_color(const glm::vec4& clear_color) { clear_color_ = clear_color; }
	// Submit a draw call
	void submit(const DrawCall& draw_call);
	// Sort queue by sorting key
	void sort();
	// Dispatch all commands
	void flush();
	// Clear queue
	void reset();

private:
	SortKey::Order order_;
	glm::vec4 clear_color_;
	CommandBuffer command_buffer_;
};

inline void MainRenderer::submit(hash_t queue, const DrawCall& dc)
{
	get_queue(queue).submit(dc);
}

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
		void* UBO_data;

		ShaderHandle shader;
		VertexArrayHandle VAO;
		UniformBufferHandle UBO;
		TextureHandle texture;

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

	DrawCall(DrawCallType dc_type, ShaderHandle shader, VertexArrayHandle VAO, uint32_t count=0, uint32_t offset=0)
	{
		type            = dc_type;
		data.shader     = shader;
		data.VAO        = VAO;
		data.UBO_data   = nullptr;
		data.count      = count;
		data.offset     = offset;
		instance_data.SSBO_data  = nullptr;
		instance_data.SSBO_size  = 0;
		key.shader = data.shader.index; // TODO: Find a way to avoid overflow when shader index can be greater than 255
	}

	// Set the full render state associated to this draw call
	inline void set_state(uint64_t state)
	{
		// W_ASSERT(state.render_target.index<256, "Framebuffer index out of bounds in view ID sorting key section.");
		// W_ASSERT(state.render_target.is_valid(), "Invalid FramebufferHandle!");
		data.state_flags = state;
		// Extract render target ID to use as view ID
		key.view = uint8_t((state & k_framebuffer_mask) >> k_framebuffer_shift);
	}

	// Setup a UBO configuration for this specific draw call
	inline void set_per_instance_UBO(UniformBufferHandle ubo, void* UBO_data, uint32_t size, DataOwnership copy)
	{
		data.UBO_data = UBO_data;
		data.UBO = ubo;

		if(UBO_data && copy)
		{
			data.UBO_data = W_NEW_ARRAY_DYNAMIC(uint8_t, size, MainRenderer::get_arena());
			memcpy(data.UBO_data, UBO_data, size);
		}
	}

	// Set instance data array containing all information necessary to render instance_count instances of the same geometry
	// Only available for instanced draw calls
	inline void set_instance_data_SSBO(ShaderStorageBufferHandle ssbo, void* SSBO_data, uint32_t size, uint32_t inst_count, DataOwnership copy)
	{
		W_ASSERT(type == DrawCall::IndexedInstanced || type == DrawCall::ArrayInstanced, "Cannot set instance data for non-instanced draw call.");

		instance_data.SSBO_data = SSBO_data;
		instance_data.SSBO = ssbo;
		instance_data.SSBO_size = size;
		instance_data.instance_count = inst_count;

		if(SSBO_data && copy)
		{
			instance_data.SSBO_data = W_NEW_ARRAY_DYNAMIC(uint8_t, size, MainRenderer::get_arena());
			memcpy(instance_data.SSBO_data, SSBO_data, size);
		}
	}

	// Set a single texture bound to slot 0 for this draw call
	inline void set_texture(TextureHandle tex)
	{
		W_ASSERT_FMT(tex.is_valid(), "Invalid TextureHandle of index: %hu", tex.index);
		data.texture = tex;
	}

	// Compute the sorting key for depth ascending/descending policies
	inline void set_key_depth(float depth, uint8_t layer_id)
	{
		W_ASSERT(data.shader.index<256, "Shader index out of bounds in shader sorting key section.");
		key.view |= (layer_id<<8);
		key.depth = *((uint32_t*)(&depth)); // TODO: Normalize depth and extract 24b mantissa
	}

	// Compute the sorting key for the sequential policy
	inline void set_key_sequence(uint32_t sequence, uint8_t layer_id)
	{
		W_ASSERT(data.shader.index<256, "Shader index out of bounds in shader sorting key section.");
		key.view |= (layer_id<<8);
		key.sequence = sequence;
	}
};



} // namespace erwin