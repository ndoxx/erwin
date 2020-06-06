#include "asset/texture_loader.h"
#include "filesystem/image_file.h"
#include "render/renderer.h"
#include "utils/future.hpp"

namespace erwin
{
namespace experimental
{

AssetMetaData TextureLoader::build_meta_data(const fs::path& file_path)
{
    // Sanity check
    W_ASSERT(fs::exists(file_path), "File does not exist.");
    hash_t hextension = H_(file_path.extension().string().c_str());
    bool compatible = (hextension == ".hdr"_h) || (hextension == ".png"_h);
    W_ASSERT_FMT(compatible, "Incompatible file type: %s", file_path.extension().string().c_str());

    AssetMetaData::AssetType asset_type;
    switch(hextension)
    {
    case ".png"_h:
        asset_type = AssetMetaData::AssetType::ImageFilePNG;
        break;
    case ".hdr"_h:
        asset_type = AssetMetaData::AssetType::ImageFileHDR;
        break;
    }
    return {file_path, asset_type};
}

Texture2DDescriptor TextureLoader::load_from_file(const AssetMetaData& meta_data)
{
    Texture2DDescriptor descriptor;
    if(meta_data.type == AssetMetaData::AssetType::ImageFileHDR)
    {
        // Load HDR file
        img::HDRDescriptor hdrfile{meta_data.file_path};
        img::read_hdr(hdrfile);

        DLOGI << "Width:    " << WCC('v') << hdrfile.width << std::endl;
        DLOGI << "Height:   " << WCC('v') << hdrfile.height << std::endl;
        DLOGI << "Channels: " << WCC('v') << hdrfile.channels << std::endl;

        if(2 * hdrfile.height != hdrfile.width)
        {
            DLOGW("asset") << "HDR file must be in 2:1 format (width = 2 * height) for optimal results." << std::endl;
        }

        descriptor.width = hdrfile.width;
        descriptor.height = hdrfile.height;
        descriptor.mips = 0;
        descriptor.data = hdrfile.data;
        descriptor.image_format = ImageFormat::RGB32F;
        descriptor.flags = TF_MUST_FREE; // Let the renderer free the resources once the texture is loaded
    }
    else if(meta_data.type == AssetMetaData::AssetType::ImageFilePNG)
    {
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
    }
    return descriptor;
}

TextureHandle TextureLoader::upload(const Texture2DDescriptor& descriptor)
{
    return Renderer::create_texture_2D(descriptor);
}

void TextureLoader::destroy(Resource& resource)
{
	Renderer::destroy(resource.first);
}

TextureLoader::Resource TextureLoader::managed_resource(TextureHandle handle, const Texture2DDescriptor& descriptor)
{
	return {handle, descriptor};
}

} // namespace experimental
} // namespace erwin