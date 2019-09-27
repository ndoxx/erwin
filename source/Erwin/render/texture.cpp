#include "render/texture.h"
#include "debug/logger.h"

#include "render/render_device.h"
#include "platform/ogl_texture.h"

namespace erwin
{

WRef<Texture2D> Texture2D::create(const fs::path& filepath)
{
    auto fullpath = filesystem::get_asset_dir() / filepath;

    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("texture") << "Texture2D: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return make_ref<OGLTexture2D>(fullpath);
    }
}

WRef<Texture2D> Texture2D::create(const Texture2DDescriptor& descriptor)
{
    switch(Gfx::get_api())
    {
        case GfxAPI::None:
            DLOGE("texture") << "Texture2D: not implemented for GfxAPI::None." << std::endl;
            return nullptr;

        case GfxAPI::OpenGL:
            return make_ref<OGLTexture2D>(descriptor);
    }
}


} // namespace erwin