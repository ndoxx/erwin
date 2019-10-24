#include "render/WIP/master_renderer.h"

#include <memory>
#include <iostream>

#include "render/buffer.h"
#include "debug/logger.h"
#include "memory/arena.h"
#include "memory/memory_utils.h"
#include "memory/linear_allocator.h"
#include "memory/handle_pool.h"

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
		std::fill(std::begin(index_buffers), std::end(index_buffers), nullptr);
	}

	WRef<BufferLayout> vertex_buffer_layouts[k_max_vertex_buffer_layouts];
	WRef<IndexBuffer>  index_buffers[k_max_index_buffers];
	WRef<VertexBuffer> vertex_buffers[k_max_vertex_buffers];
	WRef<VertexArray>  vertex_arrays[k_max_vertex_arrays];

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

	// IndexBufferHandle* handle;
	IndexBufferHandle handle;
	uint32_t* index_data;
	uint32_t count;
	DrawPrimitive primitive;
	DrawMode mode;

	cmd->read(&mode);
	cmd->read(&primitive);
	cmd->read(&count);
	cmd->read(&index_data);
	cmd->read(&handle);

	s_storage->index_buffers[handle.index] = IndexBuffer::create(index_data, count, primitive, mode);
}

void MasterRenderer::dispatch::create_vertex_buffer_layout(RenderCommand* cmd)
{
	W_ASSERT(cmd->type == RenderCommand::CreateVertexBufferLayout, "Wrong command type or dispatch.");

	uint32_t count;
	VertexBufferLayoutHandle handle;
	cmd->read(&count);
	cmd->read(&handle);

	s_storage->vertex_buffer_layouts[handle.index] = make_ref<BufferLayout>(reinterpret_cast<BufferLayoutElement*>(cmd->auxiliary), count);
}

void MasterRenderer::dispatch::create_vertex_buffer(RenderCommand* cmd)
{
	W_ASSERT(cmd->type == RenderCommand::CreateVertexBuffer, "Wrong command type or dispatch.");

	VertexBufferHandle handle;
	VertexBufferLayoutHandle layout_hnd;
	uint32_t count;
	DrawMode mode;
	cmd->read(&mode);
	cmd->read(&count);
	cmd->read(&layout_hnd);
	cmd->read(&handle);
	W_ASSERT(layout_hnd.is_valid(), "Invalid handle!");

	const auto& layout = *s_storage->vertex_buffer_layouts[layout_hnd.index];
	s_storage->vertex_buffers[handle.index] = VertexBuffer::create(reinterpret_cast<float*>(cmd->auxiliary), count, layout, mode);
}

void MasterRenderer::dispatch::create_vertex_array(RenderCommand* cmd)
{
	W_ASSERT(cmd->type == RenderCommand::CreateVertexArray, "Wrong command type or dispatch.");

	VertexArrayHandle handle;
	VertexBufferHandle vb;
	IndexBufferHandle ib;
	cmd->read(&ib);
	cmd->read(&vb);
	cmd->read(&handle);
	W_ASSERT(vb.is_valid(), "Invalid handle!");

	s_storage->vertex_arrays[handle.index] = VertexArray::create();
	s_storage->vertex_arrays[handle.index]->set_vertex_buffer(s_storage->vertex_buffers[vb.index]);
	if(ib.is_valid())
		s_storage->vertex_arrays[handle.index]->set_index_buffer(s_storage->index_buffers[ib.index]);
}

void MasterRenderer::dispatch::create_uniform_buffer(RenderCommand* cmd)
{

}

} // namespace WIP
} // namespace erwin