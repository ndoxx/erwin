#include "render/WIP/command_queue.h"
#include "render/WIP/master_renderer.h"

namespace erwin
{
namespace WIP
{

constexpr uint8_t  k_flags_draw_primitive_bits  = 2;
constexpr uint64_t k_flags_draw_primitive_shift = uint64_t(64) - k_flags_draw_primitive_bits;

constexpr uint8_t  k_flags_draw_mode_bits  = 2;
constexpr uint64_t k_flags_draw_mode_shift = k_flags_draw_primitive_shift - k_flags_draw_mode_bits;

void RenderCommand::create_index_buffer(IndexBufferHandle* handle, uint32_t* index_data, uint32_t count, DrawPrimitive primitive, DrawMode mode)
{
	reset();
	type = CreateIndexBuffer;
	handle->invalidate();

	// Write flags
	switch(primitive) // TMP
	{
		case DrawPrimitive::Lines:     flags |= uint64_t(0x1) << k_flags_draw_primitive_shift; break;
		case DrawPrimitive::Triangles: flags |= uint64_t(0x2) << k_flags_draw_primitive_shift; break;
		case DrawPrimitive::Quads:     flags |= uint64_t(0x3) << k_flags_draw_primitive_shift; break;
	}
	flags |= (uint64_t)mode << k_flags_draw_mode_shift;

	// Write data
	write(&handle,     sizeof(IndexBufferHandle*));
	write(&index_data, sizeof(uint32_t*));
	write(&count,      sizeof(uint32_t));
	write(&primitive,  sizeof(DrawPrimitive)); // TMP
	write(&mode,       sizeof(DrawMode)); // TMP

	// Set dispatch function
	backend_dispatch_func = MasterRenderer::dispatch::create_index_buffer;
}



} // namespace WIP
} // namespace erwin