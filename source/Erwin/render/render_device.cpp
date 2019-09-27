#include "render/render_device.h"
#include "platform/ogl_render_device.h"

namespace erwin
{

GfxAPI Gfx::api_ = GfxAPI::OpenGL;
std::unique_ptr<RenderDevice> Gfx::device = std::make_unique<OGLRenderDevice>();
std::unique_ptr<FramebufferPool> Gfx::framebuffer_pool;

void Gfx::set_api(GfxAPI api)
{
    api_ = api;

    if(api_ == GfxAPI::OpenGL)
        device = std::make_unique<OGLRenderDevice>();
}

void Gfx::create_framebuffer_pool(uint32_t width, uint32_t height)
{
    framebuffer_pool = std::make_unique<FramebufferPool>(width, height);
}


} // namespace erwin