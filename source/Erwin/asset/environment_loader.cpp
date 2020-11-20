#include "asset/environment_loader.h"
#include "filesystem/image_file.h"
#include "render/renderer.h"
#include "render/renderer_3d.h"

namespace erwin
{

AssetMetaData EnvironmentLoader::build_meta_data(const WPath& file_path)
{
    // Sanity check
    K_ASSERT(file_path.exists(), "File does not exist.");
    K_ASSERT_FMT(file_path.check_extension(".hdr"_h), "Incompatible file type: %s", file_path.extension().c_str());

    return {file_path, AssetMetaData::AssetType::EnvironmentHDR};
}

Texture2DDescriptor EnvironmentLoader::load_from_file(const AssetMetaData& meta_data)
{
    KLOG("asset", 1) << "Loading environment:" << std::endl;
    KLOGI << kb::WCC('p') << meta_data.file_path << std::endl;

    Texture2DDescriptor descriptor;

    // Load HDR file
    img::HDRDescriptor hdrfile{meta_data.file_path};
    img::read_hdr(hdrfile);

    KLOGI << "Width:    " << kb::WCC('v') << hdrfile.width << std::endl;
    KLOGI << "Height:   " << kb::WCC('v') << hdrfile.height << std::endl;
    KLOGI << "Channels: " << kb::WCC('v') << hdrfile.channels << std::endl;

    if(2 * hdrfile.height != hdrfile.width)
    {
        KLOGW("asset") << "HDR file must be in 2:1 format (width = 2 * height) for optimal results." << std::endl;
    }

    descriptor.width = hdrfile.width;
    descriptor.height = hdrfile.height;
    descriptor.mips = 0;
    descriptor.data = hdrfile.data;
    descriptor.image_format = ImageFormat::RGB32F;
    descriptor.flags = TF_MUST_FREE; // Let the renderer free the resources once the texture is loaded

    return descriptor;
}

Environment EnvironmentLoader::upload(const Texture2DDescriptor& descriptor, hash_t resource_id)
{
    Environment environment;
    TextureHandle handle = Renderer::create_texture_2D(descriptor);
    environment.size = descriptor.height;
    environment.environment_map = Renderer3D::generate_cubemap_hdr(handle, environment.size);
    environment.diffuse_irradiance_map = Renderer3D::generate_irradiance_map(environment.environment_map);
    environment.prefiltered_map = Renderer3D::generate_prefiltered_map(environment.environment_map, environment.size);
    environment.resource_id = resource_id;
    Renderer::destroy(handle);

    return environment;
}

void EnvironmentLoader::destroy(Environment& environment)
{
    Renderer::destroy(environment.environment_map);
    Renderer::destroy(environment.diffuse_irradiance_map);
    Renderer::destroy(environment.prefiltered_map);
}

} // namespace erwin