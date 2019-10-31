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

	static void init();
	static void shutdown();

	static void set_profiling_enabled(bool value=true);
	static const MainRendererStats& get_stats();

	static FramebufferHandle default_render_target();

	static RenderQueue& get_queue(int name);
	static void flush();

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

	enum class Phase
	{
		Pre,
		Post
	};

	RenderQueue(memory::HeapArea& memory, SortKey::Order order);
	~RenderQueue();

	// * These functions change the queue state persistently
	// Set clear color for associated render target
	void set_clear_color(uint8_t R, uint8_t G, uint8_t B, uint8_t A=255);
	//
	void set_state(const PassState& state);
	//
	void set_render_target(FramebufferHandle fb);

	// * The following functions will initialize a render command and push it to this queue 
	IndexBufferHandle         create_index_buffer(uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode = DrawMode::Static);
	VertexBufferLayoutHandle  create_vertex_buffer_layout(const std::initializer_list<BufferLayoutElement>& elements);
	VertexBufferHandle        create_vertex_buffer(VertexBufferLayoutHandle layout, float* vertex_data, uint32_t count, DrawMode mode = DrawMode::Static);
	VertexArrayHandle         create_vertex_array(VertexBufferHandle vb, IndexBufferHandle ib);
	UniformBufferHandle       create_uniform_buffer(const std::string& name, void* data, uint32_t size, DrawMode mode = DrawMode::Dynamic);
	ShaderStorageBufferHandle create_shader_storage_buffer(const std::string& name, void* data, uint32_t size, DrawMode mode = DrawMode::Dynamic);
	ShaderHandle 			  create_shader(const fs::path& filepath, const std::string& name);
	TextureHandle 			  create_texture_2D(const Texture2DDescriptor& desc);
	FramebufferHandle 		  create_framebuffer(uint32_t width, uint32_t height, bool depth, bool stencil, const FramebufferLayout& layout);

	void update_index_buffer(IndexBufferHandle handle, uint32_t* data, uint32_t count);
	void update_vertex_buffer(VertexBufferHandle handle, void* data, uint32_t size);
	void update_uniform_buffer(UniformBufferHandle handle, void* data, uint32_t size);
	void update_shader_storage_buffer(ShaderStorageBufferHandle handle, void* data, uint32_t size);
	void shader_attach_uniform_buffer(ShaderHandle shader, UniformBufferHandle ubo);
	void shader_attach_storage_buffer(ShaderHandle shader, ShaderStorageBufferHandle ssbo);
	void update_framebuffer(FramebufferHandle fb, uint32_t width, uint32_t height);

	void submit(const DrawCall& draw_call);

	void destroy_index_buffer(IndexBufferHandle handle);
	void destroy_vertex_buffer_layout(VertexBufferLayoutHandle handle);
	void destroy_vertex_buffer(VertexBufferHandle handle);
	void destroy_vertex_array(VertexArrayHandle handle);
	void destroy_uniform_buffer(UniformBufferHandle handle);
	void destroy_shader_storage_buffer(ShaderStorageBufferHandle handle);
	void destroy_shader(ShaderHandle handle);
	void destroy_texture_2D(TextureHandle handle);
	void destroy_framebuffer(FramebufferHandle handle);

	// Sort queue by sorting key
	void sort();
	// Dispatch all commands
	void flush(Phase phase);
	// Clear queue
	void reset();

	// DEBUG
	inline const void* get_pre_command_buffer_ptr() const  { return pre_buffer_.storage.begin(); }
	inline const void* get_post_command_buffer_ptr() const { return post_buffer_.storage.begin(); }
	inline const void* get_auxiliary_buffer_ptr() const    { return auxiliary_arena_.get_allocator().begin(); }

private:
	// Helper func to update the key sequence and push commands to the queue
	void push_command(RenderCommand type, void* cmd);

	inline CommandBuffer& get_command_buffer(Phase phase)
	{
		switch(phase)
		{
			case Phase::Pre:  return pre_buffer_;
			case Phase::Post: return post_buffer_;
		}
	}

	inline CommandBuffer& get_command_buffer(RenderCommand command)
	{
		Phase phase = (command < RenderCommand::Post) ? Phase::Pre : Phase::Post;
		return get_command_buffer(phase);
	}

private:
	typedef memory::MemoryArena<memory::LinearAllocator, 
			    				memory::policy::SingleThread, 
			    				memory::policy::NoBoundsChecking,
			    				memory::policy::NoMemoryTagging,
			    				memory::policy::NoMemoryTracking> AuxArena;

	SortKey::Order order_;
	SortKey key_;
	uint32_t clear_color_;
	uint64_t state_flags_;
	FramebufferHandle render_target_;
	CommandBuffer pre_buffer_;
	CommandBuffer post_buffer_;
	AuxArena auxiliary_arena_;
};

/*
	TODO: 
	- Compress data size as much as possible
	- Handle textures
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

	hash_t sampler;
	TextureHandle texture; // TODO: allow multiple textures

private:
	RenderQueue& queue;
};

} // namespace erwin