#include "render/render_device.h"
#include "platform/ogl_render_device.h"

namespace erwin
{

GfxAPI Gfx::api_ = GfxAPI::None;
std::unique_ptr<RenderDevice> Gfx::device = nullptr;

void Gfx::set_api(GfxAPI api)
{
    api_ = api;

    if(api_ == GfxAPI::OpenGL)
        device = std::make_unique<OGLRenderDevice>();
}


} // namespace erwin