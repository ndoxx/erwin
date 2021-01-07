#include "material_loader.h"
#include "core/application.h"
#include "entity/component/PBR_material.h"
#include "filesystem/tom_file.h"
#include "render/renderer.h"
#include "render/renderer_3d.h"
#include "utils/future.hpp"

#include "asset/asset_manager.h"

namespace erwin
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
            KLOGE("texture") << "Unsupported compression option, defaulting to RGB8." << std::endl;
            return ImageFormat::RGB8;
        }
        }
    }
    else
    {
        KLOGE("texture") << "Only 3 or 4 color channels supported, but got: " << int(channels) << std::endl;
        KLOGI << "Defaulting to RGBA8." << std::endl;
        return ImageFormat::RGBA8;
    }
}

AssetMetaData MaterialLoader::build_meta_data(const std::string& file_path)
{
    K_ASSERT(WFS().exists(file_path), "File does not exist.");
    K_ASSERT_FMT(WFS().check_extension(file_path, ".tom"), "Incompatible file type: %s",
                 WFS().extension(file_path).c_str());

    return {file_path, AssetMetaData::AssetType::MaterialTOM};
}

tom::TOMDescriptor MaterialLoader::load_from_file(const AssetMetaData& meta_data)
{
    KLOG("asset", 1) << "Loading TOM file:" << std::endl;
    KLOGI << kb::KS_PATH_ << meta_data.file_path << std::endl;

    tom::TOMDescriptor descriptor;
    descriptor.filepath = meta_data.file_path;
    tom::read_tom(descriptor);
    return descriptor;
}

ComponentPBRMaterial MaterialLoader::upload(const tom::TOMDescriptor& descriptor, hash_t resource_id)
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
    KLOGI << "Found " << kb::KS_VALU_ << tg.texture_count << kb::KC_ << " texture maps. TextureHandles: { ";
    for(size_t ii = 0; ii < tg.texture_count; ++ii)
    {
        KLOG("texture", 1) << kb::KS_VALU_ << tg.textures[ii].index() << " ";
    }
    KLOG("texture", 1) << kb::KC_ << "}" << std::endl;
#endif

    K_ASSERT(descriptor.material_type == tom::MaterialType::PBR, "Material is not PBR.");
    K_ASSERT(descriptor.material_data_size == sizeof(ComponentPBRMaterial::MaterialData),
             "Invalid material data size.");

    ShaderHandle shader = AssetManager::load_shader("sysres://shaders/deferred_PBR.glsl");
    UniformBufferHandle ubo = AssetManager::create_material_data_buffer<ComponentPBRMaterial>();

    std::string name = WFS().regular_path(descriptor.filepath).stem();

    Material mat = {H_(name.c_str()), tg, shader, ubo, sizeof(ComponentPBRMaterial::MaterialData), resource_id};
    Renderer3D::register_shader(shader);
    Renderer::shader_attach_uniform_buffer(shader, ubo);

    ComponentPBRMaterial pbr_mat(mat, descriptor.material_data);
    delete[] descriptor.material_data;

    return pbr_mat;
}

void MaterialLoader::destroy(ComponentPBRMaterial& resource)
{
    for(uint32_t ii = 0; ii < resource.material.texture_group.texture_count; ++ii)
        Renderer::destroy(resource.material.texture_group[ii]);
}

} // namespace erwin