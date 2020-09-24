#include "render/query_timer.h"
#include "render/backend.h"
#include "debug/logger.h"

#include "platform/OGL/ogl_query_timer.h"

namespace erwin
{

WScope<QueryTimer> QueryTimer::create()
{
    switch(gfx::get_backend())
    {
        case GfxAPI::None:
            DLOGE("render") << "QueryTimer: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return make_scope<OGLQueryTimer>();
    }
}

} // namespace erwin