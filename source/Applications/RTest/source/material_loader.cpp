#include "material_loader.h"
#include "entity/component_PBR_material.h"
#include "render/renderer.h"
#include "render/renderer_3d.h"
#include "utils/future.hpp"

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

const ComponentPBRMaterial& MaterialLoader::load(const fs::path& file_path)
{
    W_PROFILE_FUNCTION()

    // Check cache first
    hash_t hname = H_(file_path.string().c_str());
    auto findit = managed_resources_.find(hname);
    if(findit == managed_resources_.end())
    {
        DLOG("asset", 1) << "Loading PBR material:" << std::endl;
        DLOG("asset", 1) << WCC('p') << file_path << WCC(0) << std::endl;

        AssetMetaData meta_data = build_meta_data(file_path);
        auto descriptor = load_from_file(meta_data);
        managed_resources_[hname] = upload(descriptor);
        return managed_resources_[hname];
    }
    else
    {
        DLOG("asset", 1) << "Getting PBR material " << WCC('i') << "from cache" << WCC(0) << ":" << std::endl;
        DLOG("asset", 1) << WCC('p') << file_path << WCC(0) << std::endl;

        return findit->second;
    }
}

hash_t MaterialLoader::load_async(const fs::path& file_path)
{
    W_PROFILE_FUNCTION()

    // Check cache first
    hash_t hname = H_(file_path.string().c_str());
    if(managed_resources_.find(hname) == managed_resources_.end())
    {
        DLOG("asset", 1) << "Loading PBR material (async):" << std::endl;
        DLOG("asset", 1) << WCC('p') << file_path << WCC(0) << std::endl;

        auto&& [token, fut] = promises_.future_operation();
        AssetMetaData meta_data = build_meta_data(file_path);
        file_loading_tasks_.push_back(FileLoadingTask{token, meta_data});
        upload_tasks_.push_back(UploadTask{token, meta_data, std::move(fut)});
    }

    return hname;
}

void MaterialLoader::on_ready(hash_t future_mat, std::function<void(const ComponentPBRMaterial&)> then)
{
    after_upload_tasks_.push_back(AfterUploadTask{future_mat, then});
}

void MaterialLoader::release(hash_t hname)
{
    W_PROFILE_FUNCTION()

    auto it = managed_resources_.find(hname);
    if(it != managed_resources_.end())
    {
        auto& mat = it->second;
        for(size_t ii = 0; ii < mat.material.texture_group.texture_count; ++ii)
            Renderer::destroy(mat.material.texture_group[ii]);
    }
    managed_resources_.erase(it);
}

void MaterialLoader::launch_async_tasks()
{
    // TMP: single thread loading all resources
    std::thread task([&]() {
        for(auto&& task : file_loading_tasks_)
        {
            DLOGN("asset") << "Loading PBR material (async):" << std::endl;
            DLOG("asset", 1) << WCC('p') << task.meta_data.file_path << WCC(0) << std::endl;

            tom::TOMDescriptor descriptor;
            descriptor.filepath = task.meta_data.file_path;
            tom::read_tom(descriptor);

            promises_.fulfill(task.token, std::move(descriptor));
        }
        file_loading_tasks_.clear();
    });
    task.detach();
}

void MaterialLoader::update()
{
    W_PROFILE_FUNCTION()

    for(auto it = upload_tasks_.begin(); it != upload_tasks_.end();)
    {
        auto&& task = *it;
        if(is_ready(task.future_tom))
        {
            auto&& descriptor = task.future_tom.get();

            hash_t hname = H_(task.meta_data.file_path.string().c_str());
            managed_resources_[hname] = upload(descriptor);
            upload_tasks_.erase(it);
        }
        else
            ++it;
    }

    for(auto it = after_upload_tasks_.begin(); it != after_upload_tasks_.end();)
    {
        auto&& task = *it;
        auto findit = managed_resources_.find(task.name);
        if(findit != managed_resources_.end())
        {
            const ComponentPBRMaterial& pbr_mat = findit->second;
            task.init(pbr_mat);
            after_upload_tasks_.erase(it);
        }
        else
            ++it;
    }
}

} // namespace experimental
} // namespace erwin