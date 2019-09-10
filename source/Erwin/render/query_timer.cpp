#include "render/query_timer.h"
#include "render/render_device.h"
#include "debug/logger.h"

#include "platform/ogl_query_timer.h"

namespace erwin
{

QueryTimer* QueryTimer::create()
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("render") << "QueryTimer: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return new OGLQueryTimer();
    }
}

} // namespace erwin