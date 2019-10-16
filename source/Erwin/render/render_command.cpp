#include "render/render_command.h"
#include "render/render_device.h"

#include <cstring>

namespace erwin
{

PassState RenderCommand::s_prev_state;

void RenderCommand::set_clear_color(const glm::vec4& color)
{
	Gfx::device->set_clear_color(color.r, color.g, color.b, color.a);
}

void RenderCommand::set_render_target(hash_t name)
{
	if(name)
		Gfx::framebuffer_pool->bind(name);
	else
		Gfx::device->bind_default_frame_buffer();
}

void RenderCommand::set_render_state(const PassState& state)
{
	set_render_target(state.render_target);
	set_clear_color(state.rasterizer_state.clear_color);

	Gfx::device->set_cull_mode(state.rasterizer_state.cull_mode);

	if(state.blend_state == BlendState::Alpha)
		Gfx::device->set_std_blending();
	else
		Gfx::device->disable_blending();


	Gfx::device->set_stencil_test_enabled(state.depth_stencil_state.stencil_test_enabled);
	if(state.depth_stencil_state.stencil_test_enabled)
	{
		Gfx::device->set_stencil_func(state.depth_stencil_state.stencil_func);
		Gfx::device->set_stencil_operator(state.depth_stencil_state.stencil_operator);
	}

	Gfx::device->set_depth_test_enabled(state.depth_stencil_state.depth_test_enabled);
	if(state.depth_stencil_state.depth_test_enabled)
		Gfx::device->set_depth_func(state.depth_stencil_state.depth_func);
}

void RenderCommand::clear(int flags)
{
	Gfx::device->clear(flags);
}

void RenderCommand::draw_indexed(const WRef<VertexArray>& vertexArray,
                         		 uint32_t count,
                         		 std::size_t offset)
{
	Gfx::device->draw_indexed(*vertexArray, count, offset);
}

void RenderCommand::draw_array(const WRef<VertexArray>& vertexArray,
                       		   DrawPrimitive prim,
                       		   uint32_t count,
                       		   std::size_t offset)
{
	Gfx::device->draw_array(*vertexArray, prim, count, offset);
}

void RenderCommand::draw_indexed_instanced(const WRef<VertexArray>& vertexArray,
                                   		   uint32_t instance_count,
                                   		   uint32_t elements_count,
                                   		   std::size_t offset)
{
	Gfx::device->draw_indexed_instanced(*vertexArray, instance_count, elements_count, offset);
}


} // namespace erwin