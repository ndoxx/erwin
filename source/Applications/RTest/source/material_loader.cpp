#include "material_loader.h"
#include "render/renderer.h"
#include "render/renderer_3d.h"
#include "utils/future.hpp"
#include "filesystem/tom_file.h"
#include "entity/component_PBR_material.h"

#include "asset_manager_exp.h"

namespace erwin
{
namespace experimental
{

// Helper func to choose the internal format as a function of color channels, compression options and srgb
static ImageFormat select_image_format(uint8_t channels, TextureCompression compression, bool srgb)
{
    if(channels == 4)
    {
        switch(compression)
        {
        case TextureCompression::None:
            return srgb ? ImageFormat::SRGB_ALPHA : ImageFormat::RGBA8;
        case TextureCompression::DXT1:
            return srgb ? ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT1 : ImageFormat::COMPRESSED_RGBA_S3TC_DXT1;
        case TextureCompression::DXT5:
            return srgb ? ImageFormat::COMPRESSED_SRGB_ALPHA_S3TC_DXT5 : ImageFormat::COMPRESSED_RGBA_S3TC_DXT5;
        }
    }
    else if(channels == 3)
    {
        switch(compression)
        {
        case TextureCompression::None:
            return srgb ? ImageFormat::SRGB8 : ImageFormat::RGB8;
        case TextureCompression::DXT1:
            return srgb ? ImageFormat::COMPRESSED_SRGB_S3TC_DXT1 : ImageFormat::COMPRESSED_RGB_S3TC_DXT1;
        default: {
            DLOGE("texture") << "Unsupported compression option, defaulting to RGB8." << std::endl;
            return ImageFormat::RGB8;
        }
        }
    }
    else
    {
        DLOGE("texture") << "Only 3 or 4 color channels supported, but got: " << int(channels) << std::endl;
        DLOGI << "Defaulting to RGBA8." << std::endl;
        return ImageFormat::RGBA8;
    }
}

AssetMetaData MaterialLoader::build_meta_data(const fs::path& file_path)
{
    W_ASSERT(fs::exists(file_path), "File does not exist.");
    W_ASSERT(!file_path.extension().string().compare(".tom"), "Invalid input file.");

    return {file_path, AssetMetaData::AssetType::MaterialTOM};
}

tom::TOMDescriptor MaterialLoader::load_from_file(const AssetMetaData& meta_data)
{
    tom::TOMDescriptor descriptor;
    descriptor.filepath = meta_data.file_path;
    tom::read_tom(descriptor);
    return descriptor;
}

ComponentPBRMaterial MaterialLoader::upload(const tom::TOMDescriptor& descriptor)
{
    W_PROFILE_FUNCTION()

    TextureGroup tg;
    // Create and register all texture maps
    for(auto&& tmap : descriptor.texture_maps)
    {
        ImageFormat format = select_image_format(tmap.channels, tmap.compression, tmap.srgb);
        TextureHandle tex = Renderer::create_texture_2D(Texture2DDescriptor{
            descriptor.width, descriptor.height, 3, tmap.data, format, tmap.filter, descriptor.address_UV,
            TF_MUST_FREE}); // Let the renderer free the resources once the texture is loaded
        tg.textures[tg.texture_count++] = tex;
    }

#ifdef W_DEBUG
    DLOGI << "Found " << WCC('v') << tg.texture_count << WCC(0) << " texture maps. TextureHandles: { ";
    for(size_t ii = 0; ii < tg.texture_count; ++ii)
    {
        DLOG("texture", 1) << WCC('v') << tg.textures[ii].index << " ";
    }
    DLOG("texture", 1) << WCC(0) << "}" << std::endl;
#endif

    W_ASSERT(descriptor.material_type == tom::MaterialType::PBR, "Material is not PBR.");
    W_ASSERT(descriptor.material_data_size == sizeof(ComponentPBRMaterial::MaterialData),
             "Invalid material data size.");

    ShaderHandle shader = AssetManager::load_shader("shaders/deferred_PBR.glsl");
    UniformBufferHandle ubo = AssetManager::create_material_data_buffer<ComponentPBRMaterial>();

    std::string name = descriptor.filepath.stem().string();

    Material mat = {H_(name.c_str()), tg, shader, ubo, sizeof(ComponentPBRMaterial::MaterialData)};
    Renderer3D::register_shader(shader);
    Renderer::shader_attach_uniform_buffer(shader, ubo);

    ComponentPBRMaterial pbr_mat;
    pbr_mat.set_material(mat);
    memcpy(&pbr_mat.material_data, descriptor.material_data, descriptor.material_data_size);
    delete[] descriptor.material_data;

    return pbr_mat;
}

void MaterialLoader::destroy(ComponentPBRMaterial& resource)
{
    for(size_t ii = 0; ii < resource.material.texture_group.texture_count; ++ii)
        Renderer::destroy(resource.material.texture_group[ii]);
}

ComponentPBRMaterial MaterialLoader::managed_resource(const ComponentPBRMaterial& resource, const tom::TOMDescriptor&)
{
	return resource;
}

} // namespace experimental
} // namespace erwin