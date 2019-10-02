#include "render/texture.h"
#include "debug/logger.h"

#include "render/render_device.h"
#include "platform/ogl_texture.h"
#include "core/tom_file.h"
#include "core/intern_string.h"

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

// Select the appropriate image format given some parameters from the descriptor
static ImageFormat select_image_format(uint8_t channels, TextureCompression compression, bool srgb)
{
    // TODO: A decision tree would be better?
    if(channels == 1)
    {
        return ImageFormat::R8;
    }
    else if(channels == 2)
    {
        return ImageFormat::RG16F;
    }
    else if(channels == 3)
    {
        if(compression == TextureCompression::None)
            return ImageFormat::RGB8;
        else if(compression == TextureCompression::DXT1 && !srgb)
            return ImageFormat::COMPRESSED_RGB_S3TC_DXT1;
        else if(compression == TextureCompression::DXT1 && srgb)
            return ImageFormat::COMPRESSED_SRGB_S3TC_DXT1;
    }
    else if (channels == 4)
    {
        if(compression == TextureCompression::None && !srgb)
            return ImageFormat::RGBA8;
        if(compression == TextureCompression::None && srgb)
            return ImageFormat::SRGB_ALPHA;
        else if(compression == TextureCompression::DXT5 && !srgb)
            return ImageFormat::COMPRESSED_RGBA_S3TC_DXT5;
        else if(compression == TextureCompression::DXT5 && srgb)
            return ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5;
    }

    return ImageFormat::NONE;
}

std::unordered_map<hash_t, WRef<Texture2D>> Texture2D::create_maps(const fs::path& filepath)
{
    DLOGN("texture") << "Loading texture from TOM file: " << std::endl;
    DLOGI << "path: " << WCC('p') << filepath << std::endl;

    fs::path fullpath = filesystem::get_asset_dir() / filepath;

    // Sanity check
    std::unordered_map<hash_t, WRef<Texture2D>> textures;
    if(fullpath.extension().string().compare(".tom"))
    {
        DLOGE("texture") << "File is not a valid .tom file." << std::endl;
        return textures;
    }

    if(!fs::exists(fullpath))
    {
        DLOGE("texture") << "File does not exist!" << std::endl;
        return textures;
    }

    // Read descriptor
    tom::TOMDescriptor desc {fullpath};
    tom::read_tom(desc);

    // For each texture map, create a texture and insert into container
    for(auto&& tmap: desc.texture_maps)
    {
        DLOGI << "Texture map: " << WCC('n') << istr::resolve(tmap.name) << std::endl;
        
        ImageFormat format = select_image_format(tmap.channels, tmap.compression, tmap.srgb);
        auto&& texture = create(Texture2DDescriptor{desc.width,
                                                    desc.height,
                                                    tmap.data,
                                                    format,
                                                    tmap.filter,
                                                    desc.address_UV,
                                                    false});
        textures.insert(std::make_pair(tmap.name, texture));
    }

    desc.release();
    return textures;
}

} // namespace erwin