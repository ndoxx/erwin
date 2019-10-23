#include "render/WIP/command_queue.h"
#include "render/WIP/master_renderer.h"

namespace erwin
{
namespace WIP
{

namespace detail
{
	inline void nop(RenderCommand*) { }
}


	// state_handler_func = MasterRenderer::dispatch::apply_state();


CommandQueue::CommandQueue(std::pair<void*,void*> mem_range, std::pair<void*,void*> aux_mem_range):
arena_(mem_range),
auxiliary_arena_(aux_mem_range),
head_(0)
{

}

CommandQueue::~CommandQueue()
{

}

void CommandQueue::reset()
{
	arena_.get_allocator().reset();
	auxiliary_arena_.get_allocator().reset();
	commands_.clear();
	head_ = 0;
}

void CommandQueue::create_index_buffer(uint64_t key, IndexBufferHandle* handle, uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode)
{
	RenderCommand* cmd = get();
	cmd->reset();
	cmd->type = RenderCommand::CreateIndexBuffer;
	handle->invalidate();

	// Write data
	cmd->write(&handle,     sizeof(IndexBufferHandle*));
	cmd->write(&index_data, sizeof(uint32_t*));
	cmd->write(&count,      sizeof(uint32_t));
	cmd->write(&primitive,  sizeof(DrawPrimitive));
	cmd->write(&mode,       sizeof(DrawMode));

	// Set dispatch functions
	cmd->backend_dispatch_func = MasterRenderer::dispatch::create_index_buffer;
	cmd->state_handler_func = &detail::nop;

	push(cmd, key);
}

void CommandQueue::create_vertex_buffer_layout(uint64_t key, VertexBufferLayoutHandle* handle, const std::initializer_list<BufferLayoutElement>& elements)
{
	RenderCommand* cmd = get();
	cmd->reset();
	cmd->type = RenderCommand::CreateVertexBufferLayout;
	handle->invalidate();

	std::vector<BufferLayoutElement> elts(elements);
	uint32_t count = elts.size();

	// Write data
	cmd->write(&handle, sizeof(VertexBufferLayoutHandle*));
	cmd->write(&count,  sizeof(uint32_t));

	// Write auxiliary data
	cmd->auxiliary = W_NEW_ARRAY_DYNAMIC(BufferLayoutElement, elts.size(), auxiliary_arena_);
	memcpy(cmd->auxiliary, elts.data(), elts.size() * sizeof(BufferLayoutElement));

	// Set dispatch functions
	cmd->backend_dispatch_func = MasterRenderer::dispatch::create_vertex_buffer_layout;
	cmd->state_handler_func = &detail::nop;

	push(cmd, key);
}

void CommandQueue::create_vertex_buffer(uint64_t key, VertexBufferHandle* handle, VertexBufferLayoutHandle* layout, float* vertex_data, uint32_t count, DrawMode mode)
{
	RenderCommand* cmd = get();
	cmd->reset();
	cmd->type = RenderCommand::CreateVertexBuffer;
	handle->invalidate();

	// Write data
	cmd->write(&handle, sizeof(VertexBufferHandle*));
	cmd->write(&layout, sizeof(VertexBufferLayoutHandle*));
	cmd->write(&count,  sizeof(uint32_t));
	cmd->write(&mode,   sizeof(DrawMode));

	// Write auxiliary data
	cmd->auxiliary = W_NEW_ARRAY_DYNAMIC(float, count, auxiliary_arena_);
	memcpy(cmd->auxiliary, vertex_data, count * sizeof(float));

	// Set dispatch functions
	cmd->backend_dispatch_func = MasterRenderer::dispatch::create_vertex_buffer;
	cmd->state_handler_func = &detail::nop;

	push(cmd, key);
}


} // namespace WIP
} // namespace erwin