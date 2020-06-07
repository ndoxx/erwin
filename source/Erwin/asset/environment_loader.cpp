#include "asset/environment_loader.h"
#include "filesystem/image_file.h"
#include "render/renderer.h"
#include "render/renderer_3d.h"

namespace erwin
{

AssetMetaData EnvironmentLoader::build_meta_data(const fs::path& file_path)
{
    // Sanity check
    W_ASSERT(fs::exists(file_path), "File does not exist.");
    hash_t hextension = H_(file_path.extension().string().c_str());
    bool compatible = (hextension == ".hdr"_h);
    W_ASSERT_FMT(compatible, "Incompatible file type: %s", file_path.extension().string().c_str());

    return {file_path, AssetMetaData::AssetType::EnvironmentHDR};
}

EnvironmentLoader::DataDescriptor EnvironmentLoader::load_from_file(const AssetMetaData& meta_data)
{
    DataDescriptor descriptor;

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

    return descriptor;
}

EnvironmentLoader::Resource EnvironmentLoader::upload(const DataDescriptor& descriptor)
{
	Environment environment;
    TextureHandle handle = Renderer::create_texture_2D(descriptor);
    environment.size = descriptor.height;
    environment.environment_map = Renderer3D::generate_cubemap_hdr(handle, environment.size);
    Renderer::destroy(handle);
    environment.diffuse_irradiance_map = Renderer3D::generate_irradiance_map(environment.environment_map);
    environment.prefiltered_map = Renderer3D::generate_prefiltered_map(environment.environment_map, environment.size);

    return environment;
}

void EnvironmentLoader::destroy(Resource& environment)
{
    Renderer::destroy(environment.environment_map);
    Renderer::destroy(environment.diffuse_irradiance_map);
    Renderer::destroy(environment.prefiltered_map);
}

EnvironmentLoader::Resource EnvironmentLoader::managed_resource(const Resource& resource, const DataDescriptor&)
{
	return resource;
}


}