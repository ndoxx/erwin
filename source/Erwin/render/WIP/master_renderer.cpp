#include "render/WIP/master_renderer.h"

#include <memory>
#include <iostream>

#include "render/buffer.h"
#include "memory/memory.hpp"
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
		VertexBuffer,
		VertexArray,
		UniformBuffer,
		ShaderStorageBuffer,
		Texture,
		Shader
	};

	static GEN_HANDLE_CONVERT_FUNC(IndexBuffer);
	static GEN_HANDLE_CONVERT_FUNC(VertexBuffer);
	static GEN_HANDLE_CONVERT_FUNC(VertexArray);
	static GEN_HANDLE_CONVERT_FUNC(UniformBuffer);
	static GEN_HANDLE_CONVERT_FUNC(ShaderStorageBuffer);
	static GEN_HANDLE_CONVERT_FUNC(Texture);
	static GEN_HANDLE_CONVERT_FUNC(Shader);

	uint16_t type;
	uint16_t index;
};

typedef memory::MemoryArena<memory::LinearAllocator, 
		    				memory::policy::SingleThread, 
		    				memory::policy::SimpleBoundsChecking,
		    				memory::policy::NoMemoryTagging,
		    				memory::policy::SimpleMemoryTracking> LinArena;

struct RendererStorage
{
	RendererStorage(size_t mem_amt):
	renderer_memory(mem_amt),
	handle_arena(renderer_memory.require_block(512_kB))
	{

	}

	std::vector<WRef<IndexBuffer>> index_buffers;

	memory::HeapArea renderer_memory;
	LinArena handle_arena;
};
std::unique_ptr<RendererStorage> s_storage;


void MasterRenderer::init()
{
	s_storage = std::make_unique<RendererStorage>(1_MB);
}

void MasterRenderer::shutdown()
{
	RendererStorage* rs = s_storage.release();
	delete rs;
}

void MasterRenderer::test_submit(RenderCommand* cmd)
{
	(*cmd->backend_dispatch_func)(cmd);
}

void MasterRenderer::dispatch::create_index_buffer(RenderCommand* cmd)
{
	W_ASSERT(cmd->type == RenderCommand::CreateIndexBuffer, "Wrong command type or dispatch.");

	IndexBufferHandle* handle;
	uint32_t* index_data;
	uint32_t count;
	DrawPrimitive primitive;
	DrawMode mode;

	cmd->read(&mode,       sizeof(DrawMode)); // TMP?
	cmd->read(&primitive,  sizeof(DrawPrimitive)); // TMP?
	cmd->read(&count,      sizeof(uint32_t));
	cmd->read(&index_data, sizeof(uint32_t*));
	cmd->read(&handle,     sizeof(IndexBufferHandle*));

	handle->index = s_storage->index_buffers.size();
	s_storage->index_buffers.push_back(IndexBuffer::create(index_data, count, primitive, mode));
}


} // namespace WIP
} // namespace erwin