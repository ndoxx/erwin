#pragma once

#include "render/render_state.h"
#include "core/core.h"

namespace erwin
{

class VertexArray;

class RenderCommand
{
public:
	// * Render state
    // Set whole render state at once
    static void set_render_state(const RenderState& state);

	// Set the color used to clear any framebuffer
    static void set_clear_color(float r, float g, float b, float a);
    // Clear currently bound framebuffer
    static void clear(int flags);
    // Set the current render target (framebuffer)
    static void set_render_target(hash_t name);


    // * Draw commands
    // Draw content of specified vertex array using indices
    static void draw_indexed(const WRef<VertexArray>& vertexArray,
                             uint32_t count = 0,
                             std::size_t offset = 0);
    // Draw content of vertex array using only vertex buffer data
    static void draw_array(const WRef<VertexArray>& vertexArray,
                           DrawPrimitive prim = DrawPrimitive::Triangles,
                           uint32_t count = 0,
                           std::size_t offset = 0);
    // Draw instance_count instances of content of vertex array using index buffer
    static void draw_indexed_instanced(const WRef<VertexArray>& vertexArray,
                                       uint32_t instance_count,
                                       uint32_t elements_count = 0,
                                       std::size_t offset = 0);
};


} // namespace erwin