#include "asset/texture_loader.h"
#include "filesystem/image_file.h"
#include "render/renderer.h"
#include "utils/future.hpp"

namespace erwin
{

AssetMetaData TextureLoader::build_meta_data(const fs::path& file_path)
{
    // Sanity check
    W_ASSERT(fs::exists(file_path), "File does not exist.");
    W_ASSERT_FMT(H_(file_path.extension().string().c_str()) == ".png"_h, "Incompatible file type: %s", file_path.extension().string().c_str());

    return {file_path, AssetMetaData::AssetType::ImageFilePNG};
}

Texture2DDescriptor TextureLoader::load_from_file(const AssetMetaData& meta_data,
                                                  std::optional<Texture2DDescriptor> options)
{
    DLOG("asset", 1) << "Loading image file:" << std::endl;
    DLOGI << WCC('p') << meta_data.file_path << std::endl;

    Texture2DDescriptor descriptor;

    if(options.has_value())
        descriptor = options.value();

    // Load PNG file
    img::PNGDescriptor pngfile{meta_data.file_path};
    // Force 4 channels
    pngfile.channels = 4;
    img::read_png(pngfile);

    DLOGI << "Width:    " << WCC('v') << pngfile.width << std::endl;
    DLOGI << "Height:   " << WCC('v') << pngfile.height << std::endl;
    DLOGI << "Channels: " << WCC('v') << pngfile.channels << std::endl;

    descriptor.width = pngfile.width;
    descriptor.height = pngfile.height;
    descriptor.data = pngfile.data;
    descriptor.flags = TF_MUST_FREE; // Let the renderer free the resources once the texture is loaded

    return descriptor;
}

FreeTexture TextureLoader::upload(const Texture2DDescriptor& descriptor)
{
    return {Renderer::create_texture_2D(descriptor), descriptor.width, descriptor.height};
}

void TextureLoader::destroy(Resource& resource) { Renderer::destroy(resource.handle); }

} // namespace erwin