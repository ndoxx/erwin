#include "asset/texture_loader.h"
#include "core/application.h"
#include "filesystem/image_file.h"
#include "render/renderer.h"
#include "utils/future.hpp"

namespace erwin
{

AssetMetaData TextureLoader::build_meta_data(const std::string& file_path)
{
    // Sanity check
    K_ASSERT(WFS_.exists(file_path), "File does not exist.");
    K_ASSERT_FMT(WFS_.check_extension(file_path, ".png"), "Incompatible file type: %s",
                 WFS_.extension(file_path).c_str());
    return {file_path, AssetMetaData::AssetType::ImageFilePNG};
}

Texture2DDescriptor TextureLoader::load_from_file(const AssetMetaData& meta_data,
                                                  std::optional<Texture2DDescriptor> options)
{
    KLOG("asset", 1) << "Loading image file:" << std::endl;
    KLOGI << kb::KS_PATH_ << meta_data.file_path << std::endl;

    Texture2DDescriptor descriptor;

    if(options.has_value())
        descriptor = options.value();

    // Load PNG file
    img::PNGDescriptor pngfile{meta_data.file_path};
    // Force 4 channels
    pngfile.channels = 4;
    img::read_png(pngfile);

    KLOGI << "Width:    " << kb::KS_VALU_ << pngfile.width << std::endl;
    KLOGI << "Height:   " << kb::KS_VALU_ << pngfile.height << std::endl;
    KLOGI << "Channels: " << kb::KS_VALU_ << pngfile.channels << std::endl;

    descriptor.width = pngfile.width;
    descriptor.height = pngfile.height;
    descriptor.data = pngfile.data;
    descriptor.flags = TF_MUST_FREE; // Let the renderer free the resources once the texture is loaded

    return descriptor;
}

FreeTexture TextureLoader::upload(const Texture2DDescriptor& descriptor, hash_t /*resource_id*/)
{
    return {Renderer::create_texture_2D(descriptor), descriptor.width, descriptor.height};
}

void TextureLoader::destroy(Resource& resource) { Renderer::destroy(resource.handle); }

} // namespace erwin