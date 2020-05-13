#include "render/backend.h"
#include "platform/OGL/ogl_backend.h"

namespace erwin
{

GfxAPI gfx::api_ = GfxAPI::None;
std::unique_ptr<Backend> gfx::backend = nullptr;

void gfx::set_backend(GfxAPI api)
{
    api_ = api;

    if(api_ == GfxAPI::OpenGL)
        backend = std::make_unique<OGLBackend>();
}


} // namespace erwin