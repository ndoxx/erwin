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
class MainRenderer
{
public:
	typedef memory::MemoryArena<memory::LinearAllocator, 
			    				memory::policy::SingleThread, 
			    				memory::policy::NoBoundsChecking,
			    				memory::policy::NoMemoryTagging,
			    				memory::policy::NoMemoryTracking> AuxArena;
			    				
	static RenderQueue& create_queue(const std::string& name, SortKey::Order order);
	static RenderQueue& get_queue(hash_t name);
	static AuxArena& get_arena();

	static FramebufferHandle default_render_target();
	static TextureHandle get_framebuffer_texture(FramebufferHandle handle, uint32_t index);
	static uint32_t get_framebuffer_texture_count(FramebufferHandle handle);

#ifdef W_DEBUG
	static void* get_native_texture_handle(TextureHandle handle);
	static void set_profiling_enabled(bool value=true);
	static const MainRendererStats& get_stats();
#endif

	static void flush();

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
	static void update_index_buffer(IndexBufferHandle handle, uint32_t* data, uint32_t count);
	static void update_vertex_buffer(VertexBufferHandle handle, void* data, uint32_t size);
	static void update_uniform_buffer(UniformBufferHandle handle, void* data, uint32_t size);
	static void update_shader_storage_buffer(ShaderStorageBufferHandle handle, void* data, uint32_t size);
	static void shader_attach_uniform_buffer(ShaderHandle shader, UniformBufferHandle ubo);
	static void shader_attach_storage_buffer(ShaderHandle shader, ShaderStorageBufferHandle ssbo);
	static void update_framebuffer(FramebufferHandle fb, uint32_t width, uint32_t height);
	static void clear_framebuffers();
	// POST-BUFFER -> executed after draw commands
	static void framebuffer_screenshot(FramebufferHandle fb, const fs::path& filepath);
	static void destroy(IndexBufferHandle handle);
	static void destroy(VertexBufferLayoutHandle handle);
	static void destroy(VertexBufferHandle handle);
	static void destroy(VertexArrayHandle handle);
	static void destroy(UniformBufferHandle handle);
	static void destroy(ShaderStorageBufferHandle handle);
	static void destroy(ShaderHandle handle);
	static void destroy(TextureHandle handle);
	static void destroy(FramebufferHandle handle);

private:
	friend class Application;

	static void init();
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

/*
	TODO: 
	- Compress data size as much as possible
*/
struct DrawCall
{
	enum Type: uint8_t
	{
		Indexed,
		Array,
		IndexedInstanced,
		ArrayInstanced,

		Count
	};

	#pragma pack(push,1)
	struct DrawCallData
	{
		uint64_t state_flags;
		void* UBO_data;
		void* SSBO_data;
		hash_t sampler;

		ShaderHandle shader;
		VertexArrayHandle VAO;
		UniformBufferHandle UBO;
		ShaderStorageBufferHandle SSBO;
		TextureHandle texture; // TODO: allow multiple textures
		Type type;

		uint32_t UBO_size;
		uint32_t SSBO_size;
		uint32_t count;
		uint32_t instance_count;
		uint32_t offset;
	} data;
	#pragma pack(pop)

	SortKey key;

	DrawCall(RenderQueue& queue, Type type, ShaderHandle shader, VertexArrayHandle VAO, uint32_t count=0, uint32_t offset=0);

	inline void set_state(uint64_t state)
	{
		// W_ASSERT(state.render_target.index<256, "Framebuffer index out of bounds in view ID sorting key section.");
		// W_ASSERT(state.render_target.is_valid(), "Invalid FramebufferHandle!");
		data.state_flags = state;
		// Extract render target ID to use as view ID
		key.view = uint8_t((state & k_framebuffer_mask) >> k_framebuffer_shift);
	}

	inline void set_per_instance_UBO(UniformBufferHandle ubo, void* UBO_data, uint32_t size)
	{
		data.UBO = ubo;
		data.UBO_size = size;

		if(UBO_data)
		{
			data.UBO_data = W_NEW_ARRAY_DYNAMIC(uint8_t, size, MainRenderer::get_arena());
			memcpy(data.UBO_data, UBO_data, size);
		}
	}

	inline void set_instance_data_SSBO(ShaderStorageBufferHandle ssbo, void* SSBO_data, uint32_t size, uint32_t inst_count)
	{
		data.SSBO = ssbo;
		data.SSBO_data = SSBO_data;
		data.SSBO_size = size;
		data.instance_count = inst_count;
	}

	inline void set_texture(hash_t smp, TextureHandle tex)
	{
		data.sampler = smp;
		data.texture = tex;
	}

	inline void submit()
	{
		queue.submit(*this);
	}

	inline void set_key_depth(float depth, uint8_t layer_id)
	{
		W_ASSERT(data.shader.index<256, "Shader index out of bounds in shader sorting key section.");
		key.view |= (layer_id<<8);
		key.shader = data.shader.index; // TODO: Find a way to avoid overflow when shader index can be greater than 255
		key.depth = *((uint32_t*)(&depth)); // TODO: Normalize depth and extract 24b mantissa
	}

	inline void set_key_sequence(uint32_t sequence, uint8_t layer_id)
	{
		W_ASSERT(data.shader.index<256, "Shader index out of bounds in shader sorting key section.");
		key.view |= (layer_id<<8);
		key.shader = data.shader.index; // TODO: Find a way to avoid overflow when shader index can be greater than 255
		key.sequence = sequence;
	}

private:
	RenderQueue& queue;
};



} // namespace erwin