#include "render/framebuffer.h"
#include "render/render_device.h"
#include "platform/ogl_framebuffer.h"
#include "debug/logger.h"

namespace erwin
{

WScope<Framebuffer> Framebuffer::create(uint32_t width, uint32_t height, const FramebufferLayout& layout, bool depth, bool stencil)
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "Framebuffer: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return make_scope<OGLFramebuffer>(width, height, layout, depth, stencil);
    }
}


} // namespace erwin