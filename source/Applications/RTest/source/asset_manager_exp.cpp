#include "asset_manager_exp.h"
#include "core/core.h"
#include "entity/component_PBR_material.h"
#include "filesystem/tom_file.h"
#include "utils/future.hpp"
#include "utils/promise_storage.hpp"

#include "render/renderer.h"
#include "render/renderer_3d.h"

//TMP
#include "asset/asset_manager.h"

#include <chrono>
#include <thread>

namespace erwin
{

struct AssetMetaData
{
    enum class AssetType : uint8_t
    {
        None,
        ImagePNG,
        ImageHDR,
        MaterialTOM,
        Mesh
    };

    fs::path file_path;
    AssetType type;
};

struct MaterialFileLoadingTask
{
    uint64_t token;
    AssetMetaData meta_data;
};

struct MaterialCreationTask
{
    uint64_t token;
    AssetMetaData meta_data;
    std::future<tom::TOMDescriptor> future_tom;
};

struct MaterialInitTask
{
    hash_t name;
    std::function<void(const ComponentPBRMaterial&)> init;
};

static struct
{
    PromiseStorage<tom::TOMDescriptor> tom_promises;
    std::map<hash_t, ComponentPBRMaterial> pbr_materials;
    std::map<uint64_t, hash_t> token_to_material_name;
    std::vector<MaterialFileLoadingTask> material_file_loading_tasks;
    std::vector<MaterialCreationTask> material_creation_tasks;
    std::vector<MaterialInitTask> material_init_tasks;
} s_storage;

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

static ComponentPBRMaterial create_material(const tom::TOMDescriptor& descriptor)
{
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
	for(size_t ii=0; ii<tg.texture_count; ++ii)
	{
		DLOG("texture",1) << WCC('v') << tg.textures[ii].index << " ";
	}
	DLOG("texture",1) << WCC(0) << "}" << std::endl;
#endif

	W_ASSERT(descriptor.material_type == tom::MaterialType::PBR, "[AssetManager] Material is not PBR.");
	W_ASSERT(descriptor.material_data_size == sizeof(ComponentPBRMaterial::MaterialData), "[AssetManager] Invalid material data size.");
	
	// TMP
	ShaderHandle shader     = AssetManager::load_shader("shaders/deferred_PBR.glsl");
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



uint64_t AssetManagerE::load_material_async(const fs::path& file_path)
{
    auto&& [token, fut] = s_storage.tom_promises.future_operation();
    AssetMetaData meta_data{file_path, AssetMetaData::AssetType::MaterialTOM};
    s_storage.material_file_loading_tasks.push_back(MaterialFileLoadingTask{token, meta_data});
    s_storage.material_creation_tasks.push_back(MaterialCreationTask{token, meta_data, std::move(fut)});
    s_storage.token_to_material_name[token] = H_(file_path.string().c_str());

    return token;
}

void AssetManagerE::on_material_ready(uint64_t future_mat, std::function<void(const ComponentPBRMaterial&)> then)
{
    hash_t mat_name = s_storage.token_to_material_name.at(future_mat);
    s_storage.material_init_tasks.push_back(MaterialInitTask{mat_name, then});
}

void AssetManagerE::launch_async_tasks()
{
    // TMP: single thread loading all resources
    std::thread task([&]() {
        for(auto&& task : s_storage.material_file_loading_tasks)
        {
#if 1
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1s);
#endif

            tom::TOMDescriptor descriptor;
            descriptor.filepath = task.meta_data.file_path;
            tom::read_tom(descriptor);

            s_storage.tom_promises.fulfill(task.token, std::move(descriptor));
        }
        s_storage.material_file_loading_tasks.clear();
    });
    task.detach();
}

void AssetManagerE::update()
{
    for(auto it = s_storage.material_creation_tasks.begin(); it != s_storage.material_creation_tasks.end();)
    {
        auto&& task = *it;
        if(is_ready(task.future_tom))
        {
            auto&& descriptor = task.future_tom.get();

            hash_t hname = H_(task.meta_data.file_path.string().c_str());
            s_storage.pbr_materials[hname] = create_material(descriptor);
            s_storage.material_creation_tasks.erase(it);
        }
        else
            ++it;
    }

    for(auto it = s_storage.material_init_tasks.begin(); it != s_storage.material_init_tasks.end();)
    {
        auto&& task = *it;
        auto findit = s_storage.pbr_materials.find(task.name);
        if(findit != s_storage.pbr_materials.end())
        {
            const ComponentPBRMaterial& pbr_mat = findit->second;
            task.init(pbr_mat);
            s_storage.material_init_tasks.erase(it);
        }
        else
            ++it;
    }
}

} // namespace erwin