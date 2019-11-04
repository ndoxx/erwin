#pragma once

#include <cstdint>
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

struct MainRendererStats
{
	float render_time = 0.f;
};

enum class RenderCommand: uint16_t
{
	CreateIndexBuffer,
	CreateVertexBufferLayout,
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

	Submit,

	Post,

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

class RenderQueue;
class MainRenderer
{
public:
	enum QueueName
	{
		Resource,
		Opaque,
		// OpaqueEmissive,
		// Transparent,

		Count
	};

	enum class Phase
	{
		Pre,
		Post
	};

	static void init();
	static void shutdown();

	static void set_profiling_enabled(bool value=true);
	static const MainRendererStats& get_stats();

	static FramebufferHandle default_render_target();

	static RenderQueue& get_queue(int name);
	static void flush();

	// * The following functions will initialize a render command and push it to the appropriate queue 
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
	static void destroy_index_buffer(IndexBufferHandle handle);
	static void destroy_vertex_buffer_layout(VertexBufferLayoutHandle handle);
	static void destroy_vertex_buffer(VertexBufferHandle handle);
	static void destroy_vertex_array(VertexArrayHandle handle);
	static void destroy_uniform_buffer(UniformBufferHandle handle);
	static void destroy_shader_storage_buffer(ShaderStorageBufferHandle handle);
	static void destroy_shader(ShaderHandle handle);
	static void destroy_texture_2D(TextureHandle handle);
	static void destroy_framebuffer(FramebufferHandle handle);

	static void DEBUG_test();
};

struct SortKey
{
	enum class Order: uint8_t
	{
		ByShader,
		ByDepth, // TODO: back to front / front to back options
		Sequential
	};

	uint64_t encode(SortKey::Order type) const;
	void decode(uint64_t key);

						  // -- dependencies --     -- meaning --
	uint8_t view;         // queue global state?	layer / viewport id
	uint8_t transparency; // queue type 			blending type: opaque / transparent
	uint8_t shader;       // command data / type    could mean "material ID" when I have a material system
	bool is_draw;         // command data / type 	whether or not the command performs a draw call
	uint32_t depth;       // command data / type 	depth mantissa
	uint32_t sequence;    // command data / type 	for commands to be dispatched sequentially
};

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
	// Set clear color for associated render target
	void set_clear_color(uint8_t R, uint8_t G, uint8_t B, uint8_t A=255);
	//
	void set_render_target(FramebufferHandle fb);

	void submit(const DrawCall& draw_call);

	// Sort queue by sorting key
	void sort();
	// Dispatch all commands
	void flush();
	// Clear queue
	void reset();

private:
	SortKey::Order order_;
	SortKey key_;
	uint32_t clear_color_;
	FramebufferHandle render_target_;
	CommandBuffer command_buffer_;
};

/*
	TODO: 
	- Compress data size as much as possible
*/
struct DrawCall
{
	enum Type
	{
		Indexed,
		Array,
		IndexedInstanced,
		ArrayInstanced,

		Count
	};

	DrawCall(RenderQueue& queue, Type type, ShaderHandle shader, VertexArrayHandle VAO, uint32_t count=0, uint32_t offset=0);

	inline void set_state(const PassState& state)
	{
		state_flags = state.encode();
	}

	inline void set_state(uint64_t state)
	{
		state_flags = state;
	}

	inline void set_per_instance_UBO(UniformBufferHandle ubo, void* data, uint32_t size)
	{
		UBO = ubo;
		UBO_data = data;
		UBO_size = size;
	}

	inline void set_instance_data_SSBO(ShaderStorageBufferHandle ssbo, void* data, uint32_t size, uint32_t inst_count)
	{
		SSBO = ssbo;
		SSBO_data = data;
		SSBO_size = size;
		instance_count = inst_count;
	}

	inline void set_texture(hash_t smp, TextureHandle tex)
	{
		sampler = smp;
		texture = tex;
	}

	inline void submit()
	{
		queue.submit(*this);
	}

	Type type;
	ShaderHandle shader;
	VertexArrayHandle VAO;
	UniformBufferHandle UBO;
	ShaderStorageBufferHandle SSBO;
	void* UBO_data;
	void* SSBO_data;
	uint32_t UBO_size;
	uint32_t SSBO_size;
	uint32_t count;
	uint32_t instance_count;
	uint32_t offset;
	uint64_t state_flags;

	hash_t sampler;
	TextureHandle texture; // TODO: allow multiple textures

private:
	RenderQueue& queue;
};

} // namespace erwin