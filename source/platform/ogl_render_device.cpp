#include <map>
#include <iostream>

#include "platform/ogl_render_device.h"
#include "core/core.h"
#include "render/buffer.h"
#include "glad/glad.h"

namespace erwin
{

static std::map<DrawPrimitive, GLenum> OGLPrimitive =
{
    {DrawPrimitive::Lines, GL_LINES},
    {DrawPrimitive::Triangles, GL_TRIANGLES},
    {DrawPrimitive::Quads, GL_QUADS}
};

inline bool is_power_of_2(int value)
{
    return bool((value) && ((value &(value - 1)) == 0));
}

void OGLRenderDevice::viewport(float xx, float yy, float width, float height)
{
    glViewport(xx, yy, width, height);
}

uint32_t OGLRenderDevice::get_default_framebuffer()
{
    return default_framebuffer_;
}

void OGLRenderDevice::set_default_framebuffer(uint32_t index)
{
    default_framebuffer_ = index;
}

void OGLRenderDevice::bind_default_frame_buffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, default_framebuffer_);
}

void OGLRenderDevice::read_framebuffer_rgba(uint32_t width, uint32_t height, unsigned char* pixels)
{
    set_pack_alignment(1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, pixels);
    set_pack_alignment(4);
}

void OGLRenderDevice::draw_indexed(const WRef<VertexArray>& vertexArray,
                                   uint32_t count,
								   std::size_t offset)
{
	vertexArray->bind();
	glDrawElements(OGLPrimitive[vertexArray->get_index_buffer().get_primitive()], 
				   (bool(count) ? count : vertexArray->get_index_buffer().get_count()),
				   GL_UNSIGNED_INT,
				   (void*)(offset * sizeof(GLuint)));
    vertexArray->unbind();
}

void OGLRenderDevice::draw_array(const WRef<VertexArray>& vertexArray,
                                 DrawPrimitive prim,
                                 uint32_t count,
                                 std::size_t offset)
{
    vertexArray->bind();
    glDrawArrays(OGLPrimitive[prim], 
                 offset, 
                 (bool(count) ? count : vertexArray->get_vertex_buffer().get_count()));
    vertexArray->unbind();
}

void OGLRenderDevice::draw_indexed_instanced(const WRef<VertexArray>& vertexArray,
                                             uint32_t instance_count,
                                             uint32_t elements_count,
                                             std::size_t offset)
{
    vertexArray->bind();
    glDrawElementsInstanced(OGLPrimitive[vertexArray->get_index_buffer().get_primitive()],
                            (bool(elements_count) ? elements_count : vertexArray->get_index_buffer().get_count()),
                            GL_UNSIGNED_INT,
                            (void*)(offset * sizeof(GLuint)),
                            instance_count);
    vertexArray->unbind();
}

void OGLRenderDevice::set_clear_color(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
}

void OGLRenderDevice::clear(int flags)
{
    GLenum clear_bits = 0;
    clear_bits |= (flags & CLEAR_COLOR_FLAG) ? GL_COLOR_BUFFER_BIT : 0;
    clear_bits |= (flags & CLEAR_DEPTH_FLAG) ? GL_DEPTH_BUFFER_BIT : 0;
    clear_bits |= (flags & CLEAR_STENCIL_FLAG) ? GL_STENCIL_BUFFER_BIT : 0;
    glClear(clear_bits);
}

void OGLRenderDevice::lock_color_buffer()
{
    glDrawBuffer(0);
}

void OGLRenderDevice::set_depth_lock(bool value)
{
    glDepthMask(!value);
}

void OGLRenderDevice::set_stencil_lock(bool value)
{
    glStencilMask(!value);
}

void OGLRenderDevice::set_cull_mode(CullMode value)
{
    switch(value)
    {
        case CullMode::None:
            glDisable(GL_CULL_FACE);
            break;
        case CullMode::Front:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        case CullMode::Back:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
    }
}

void OGLRenderDevice::set_std_blending()
{
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void OGLRenderDevice::set_light_blending()
{
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);
}

void OGLRenderDevice::disable_blending()
{
    glDisable(GL_BLEND);
}

void OGLRenderDevice::set_depth_func(DepthFunc value)
{
    switch(value)
    {
        case DepthFunc::Less:
            glDepthFunc(GL_LESS);
            break;

        case DepthFunc::LEqual:
            glDepthFunc(GL_LEQUAL);
            break;
    }
}

void OGLRenderDevice::set_depth_test_enabled(bool value)
{
    if(value)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

void OGLRenderDevice::set_stencil_func(StencilFunc value, uint16_t a, uint16_t b)
{
    switch(value)
    {
        case StencilFunc::Always:
            glStencilFunc(GL_ALWAYS, 0, 0);
            break;

        case StencilFunc::NotEqual:
            glStencilFunc(GL_NOTEQUAL, a, b);
            break;
    }
}

void OGLRenderDevice::set_stencil_operator(StencilOperator value)
{
    if(value == StencilOperator::LightVolume)
    {
#ifndef __OPTIM_LIGHT_VOLUMES_STENCIL_INVERT__
        // If depth test fails, back faces polygons increment the stencil value
        // else nothing changes
        glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
        // If depth test fails, front faces polygons decrement the stencil value
        // else nothing changes
        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
#else
        // If depth test fails, front and back polygons invert the stencil value bitwise
        // else nothing changes
        glStencilOp(GL_KEEP, GL_INVERT, GL_KEEP);
#endif //__OPTIM_LIGHT_VOLUMES_STENCIL_INVERT__
    }
}

void OGLRenderDevice::set_stencil_test_enabled(bool value)
{
    if(value)
        glEnable(GL_STENCIL_TEST);
    else
        glDisable(GL_STENCIL_TEST);
}

void OGLRenderDevice::finish()
{
    glFinish();
}

void OGLRenderDevice::flush()
{
    glFlush();
}

static const std::map<GLenum, std::string> s_gl_errors =
{
    {GL_NO_ERROR,          "No error"},
    {GL_INVALID_OPERATION, "Invalid operation"},
    {GL_INVALID_ENUM,      "Invalid enum"},
    {GL_INVALID_VALUE,     "Invalid value"},
    {GL_STACK_OVERFLOW,    "Stack overflow"},
    {GL_STACK_UNDERFLOW,   "Stack underflow"},
    {GL_OUT_OF_MEMORY,     "Out of memory"}
};

static const std::string s_gl_unknown_error = "Unknown error";

const std::string& OGLRenderDevice::show_error()
{
    auto it = s_gl_errors.find(glGetError());
    if(it!=s_gl_errors.end())
        return it->second;
    else
        return s_gl_unknown_error;
}

uint32_t OGLRenderDevice::get_error()
{
    return glGetError();
}

void OGLRenderDevice::assert_no_error()
{
    W_ASSERT(glGetError()==0, "OpenGL error occurred!");
}

void OGLRenderDevice::set_pack_alignment(uint32_t value)
{
    W_ASSERT(is_power_of_2(value), "OGLRenderDevice::set_pack_alignment: arg must be a power of 2.");
    glPixelStorei(GL_PACK_ALIGNMENT, value);
}

void OGLRenderDevice::set_unpack_alignment(uint32_t value)
{
    W_ASSERT(is_power_of_2(value), "OGLRenderDevice::set_pack_alignment: arg must be a power of 2.");
    glPixelStorei(GL_UNPACK_ALIGNMENT, value);
}

void OGLRenderDevice::set_line_width(float value)
{
    glLineWidth(value);
}

} // namespace erwin