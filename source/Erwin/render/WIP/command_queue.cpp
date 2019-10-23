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

void RenderCommand::create_index_buffer(IndexBufferHandle* handle, uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode)
{
	reset();
	type = CreateIndexBuffer;
	handle->invalidate();

	// Write data
	write(&handle,     sizeof(IndexBufferHandle*));
	write(&index_data, sizeof(uint32_t*));
	write(&count,      sizeof(uint32_t));
	write(&primitive,  sizeof(DrawPrimitive));
	write(&mode,       sizeof(DrawMode));

	// Set dispatch functions
	backend_dispatch_func = MasterRenderer::dispatch::create_index_buffer;
	state_handler_func = &detail::nop;
}

	// state_handler_func = MasterRenderer::dispatch::apply_state();


CommandQueue::CommandQueue(std::pair<void*,void*> mem_range):
arena_(mem_range),
head_(0)
{

}

CommandQueue::~CommandQueue()
{

}

void CommandQueue::reset()
{
	arena_.get_allocator().reset();
	commands_.clear();
	head_ = 0;
}


} // namespace WIP
} // namespace erwin