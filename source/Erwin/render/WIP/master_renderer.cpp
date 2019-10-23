#include "render/WIP/master_renderer.h"

#include <memory>
#include <iostream>

#include "render/buffer.h"
#include "debug/logger.h"
#include "memory/arena.h"
#include "memory/memory_utils.h"
#include "memory/linear_allocator.h"

namespace erwin
{
namespace WIP
{

#define GEN_HANDLE_CONVERT_FUNC(name)             \
	inline GenHandle convert(name##Handle handle) \
	{                                             \
		return { GenHandle::name, handle.index }; \
	}

struct GenHandle
{
	enum: uint16_t
	{
		IndexBuffer,
		VertexBufferLayout,
		VertexBuffer,
		VertexArray,
		UniformBuffer,
		ShaderStorageBuffer,
		Texture,
		Shader
	};

	static GEN_HANDLE_CONVERT_FUNC(IndexBuffer);
	static GEN_HANDLE_CONVERT_FUNC(VertexBufferLayout);
	static GEN_HANDLE_CONVERT_FUNC(VertexBuffer);
	static GEN_HANDLE_CONVERT_FUNC(VertexArray);
	static GEN_HANDLE_CONVERT_FUNC(UniformBuffer);
	static GEN_HANDLE_CONVERT_FUNC(ShaderStorageBuffer);
	static GEN_HANDLE_CONVERT_FUNC(Texture);
	static GEN_HANDLE_CONVERT_FUNC(Shader);

	uint16_t type;
	uint32_t index;
};

struct RendererStorage
{
	RendererStorage():
	renderer_memory(10_MB),
	handle_arena(renderer_memory.require_block(512_kB))
	{

	}

	std::vector<WRef<IndexBuffer>> index_buffers;
	std::vector<WRef<VertexBuffer>> vertex_buffers;
	std::vector<BufferLayout> vertex_buffer_layouts;

	std::vector<CommandQueue> queues_;

	memory::HeapArea renderer_memory;
	LinearArena handle_arena; // TODO: Use pool allocator instead when we have one
};
std::unique_ptr<RendererStorage> s_storage;


void MasterRenderer::init()
{
	DLOGN("render") << "[MasterRenderer] Allocating renderer storage." << std::endl;
	// Create storage object
	s_storage = std::make_unique<RendererStorage>();
	// Create command queues
	for(int queue_name = 0; queue_name < CommandQueue::Count; ++queue_name)
		s_storage->queues_.emplace_back(s_storage->renderer_memory.require_block(512_kB), // For commands
										s_storage->renderer_memory.require_block(2_MB));  // For auxiliary data
	DLOGI << "done" << std::endl;
}

void MasterRenderer::shutdown()
{
	DLOGN("render") << "[MasterRenderer] Releasing renderer storage." << std::endl;
	RendererStorage* rs = s_storage.release();
	delete rs;
	DLOGI << "done" << std::endl;
}

CommandQueue& MasterRenderer::get_queue(int name)
{
	W_ASSERT(name < CommandQueue::Count, "Unknown queue name!");
	return s_storage->queues_[name];
}

void MasterRenderer::flush()
{
	// Sort and flush each queue
	for(int queue_name = 0; queue_name < CommandQueue::Count; ++queue_name)
	{
		auto& queue = s_storage->queues_[queue_name];
		queue.sort();
		queue.flush();
		queue.reset();
	}
}

void MasterRenderer::test_submit(RenderCommand* cmd)
{
	(*cmd->backend_dispatch_func)(cmd);
}

void MasterRenderer::test()
{
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(s_storage->renderer_memory.begin())+512_kB, 512_B);
	memory::hex_dump(std::cout, reinterpret_cast<uint8_t*>(s_storage->renderer_memory.begin())+1_MB, 512_B);
}

void MasterRenderer::dispatch::create_index_buffer(RenderCommand* cmd)
{
	W_ASSERT(cmd->type == RenderCommand::CreateIndexBuffer, "Wrong command type or dispatch.");

	IndexBufferHandle* handle;
	uint32_t* index_data;
	uint32_t count;
	DrawPrimitive primitive;
	DrawMode mode;

	cmd->read(&mode,       sizeof(DrawMode));
	cmd->read(&primitive,  sizeof(DrawPrimitive));
	cmd->read(&count,      sizeof(uint32_t));
	cmd->read(&index_data, sizeof(uint32_t*));
	cmd->read(&handle,     sizeof(IndexBufferHandle*));

	handle->index = s_storage->index_buffers.size();
	s_storage->index_buffers.push_back(IndexBuffer::create(index_data, count, primitive, mode));
}

void MasterRenderer::dispatch::create_vertex_buffer_layout(RenderCommand* cmd)
{
	W_ASSERT(cmd->type == RenderCommand::CreateVertexBufferLayout, "Wrong command type or dispatch.");

	uint32_t count;
	VertexBufferLayoutHandle* handle;
	cmd->read(&count,  sizeof(uint32_t));
	cmd->read(&handle, sizeof(VertexBufferLayoutHandle*));

	handle->index = s_storage->vertex_buffer_layouts.size();
	s_storage->vertex_buffer_layouts.emplace_back(reinterpret_cast<BufferLayoutElement*>(cmd->auxiliary), count);
}

void MasterRenderer::dispatch::create_vertex_buffer(RenderCommand* cmd)
{
	W_ASSERT(cmd->type == RenderCommand::CreateVertexBuffer, "Wrong command type or dispatch.");

	VertexBufferHandle* handle;
	VertexBufferLayoutHandle* layout_hnd;
	uint32_t count;
	DrawMode mode;
	cmd->read(&mode,       sizeof(DrawMode));
	cmd->read(&count,      sizeof(uint32_t));
	cmd->read(&layout_hnd, sizeof(VertexBufferLayoutHandle*));
	cmd->read(&handle,     sizeof(VertexBufferHandle*));
	W_ASSERT(layout_hnd->is_valid(), "Invalid handle!");

	float* vertex_data = reinterpret_cast<float*>(cmd->auxiliary);
	const auto& layout = s_storage->vertex_buffer_layouts[layout_hnd->index];

	handle->index = s_storage->vertex_buffers.size();
	s_storage->vertex_buffers.push_back(VertexBuffer::create(vertex_data, count, layout, mode));
}


namespace hnd
{
	template<> IndexBufferHandle*         get() { return W_NEW(IndexBufferHandle, s_storage->handle_arena); }
	template<> VertexBufferLayoutHandle*  get() { return W_NEW(VertexBufferLayoutHandle, s_storage->handle_arena); }
	template<> VertexBufferHandle*        get() { return W_NEW(VertexBufferHandle, s_storage->handle_arena); }
	template<> VertexArrayHandle*         get() { return W_NEW(VertexArrayHandle, s_storage->handle_arena); }
	template<> UniformBufferHandle*       get() { return W_NEW(UniformBufferHandle, s_storage->handle_arena); }
	template<> ShaderStorageBufferHandle* get() { return W_NEW(ShaderStorageBufferHandle, s_storage->handle_arena); }
	template<> TextureHandle*             get() { return W_NEW(TextureHandle, s_storage->handle_arena); }
	template<> ShaderHandle*              get() { return W_NEW(ShaderHandle, s_storage->handle_arena); }

	template<> void release(IndexBufferHandle* handle)         { W_DELETE(handle, s_storage->handle_arena); }
	template<> void release(VertexBufferLayoutHandle* handle)  { W_DELETE(handle, s_storage->handle_arena); }
	template<> void release(VertexBufferHandle* handle)        { W_DELETE(handle, s_storage->handle_arena); }
	template<> void release(VertexArrayHandle* handle)         { W_DELETE(handle, s_storage->handle_arena); }
	template<> void release(UniformBufferHandle* handle)       { W_DELETE(handle, s_storage->handle_arena); }
	template<> void release(ShaderStorageBufferHandle* handle) { W_DELETE(handle, s_storage->handle_arena); }
	template<> void release(TextureHandle* handle)             { W_DELETE(handle, s_storage->handle_arena); }
	template<> void release(ShaderHandle* handle)              { W_DELETE(handle, s_storage->handle_arena); }
} // namespace hnd


} // namespace WIP
} // namespace erwin