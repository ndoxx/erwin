#include "asset/asset_manager.h"
#include "core/intern_string.h"
#include "entity/component_PBR_material.h"
#include "render/renderer.h"
#include "utils/future.hpp"

#include "asset/atlas_loader.h"
#include "asset/environment_loader.h"
#include "asset/material_loader.h"
#include "asset/mesh_loader.h"
#include "asset/resource_manager.hpp"
#include "asset/special_texture_factory.h"
#include "asset/texture_loader.h"

#include <chrono>
#include <thread>

namespace erwin
{

static struct
{
    ResourceManager<MaterialLoader> material_manager;
    ResourceManager<TextureLoader> texture_manager;
    ResourceManager<FontAtlasLoader> font_atlas_manager;
    ResourceManager<TextureAtlasLoader> texture_atlas_manager;
    ResourceManager<EnvironmentLoader> environment_manager;
    ResourceManager<MeshLoader> mesh_manager;

    std::map<hash_t, ShaderHandle> shader_cache;
    std::map<uint64_t, UniformBufferHandle> ubo_cache;
    std::map<hash_t, TextureHandle> special_textures_cache_;
} s_storage;

UniformBufferHandle AssetManager::create_material_data_buffer(uint64_t component_id, uint32_t size)
{
    W_PROFILE_FUNCTION()

    auto it = s_storage.ubo_cache.find(component_id);
    if(it != s_storage.ubo_cache.end())
        return it->second;

    auto handle = Renderer::create_uniform_buffer("material_data", nullptr, size, UsagePattern::Dynamic);
    s_storage.ubo_cache.insert({component_id, handle});
    return handle;
}

ShaderHandle AssetManager::load_shader(const fs::path& file_path, const std::string& name)
{
    W_PROFILE_FUNCTION()

    hash_t hname = H_(file_path.string().c_str());
    auto it = s_storage.shader_cache.find(hname);
    if(it != s_storage.shader_cache.end())
        return it->second;

    DLOGN("asset") << "[AssetManager] Creating new shader:" << std::endl;

    // First, check if shader file exists in system assets
    fs::path fullpath;
    if(fs::exists(wfs::get_system_asset_dir() / file_path))
        fullpath = wfs::get_system_asset_dir() / file_path;
    else
        fullpath = wfs::get_asset_dir() / file_path;

    std::string shader_name = name.empty() ? file_path.stem().string() : name;
    ShaderHandle handle = Renderer::create_shader(fullpath, shader_name);
    DLOG("asset", 1) << "ShaderHandle: " << WCC('v') << handle.index << std::endl;
    s_storage.shader_cache.insert({hname, handle});

    return handle;
}

TextureHandle AssetManager::create_debug_texture(hash_t type, uint32_t size_px)
{
    W_PROFILE_FUNCTION()

    // First, check cache
    hash_t hname = HCOMBINE_(type, hash_t(size_px));
    auto it = s_storage.special_textures_cache_.find(hname);
    if(it != s_storage.special_textures_cache_.end())
        return it->second;

    // Create checkerboard pattern
    uint8_t* buffer = new uint8_t[size_px * size_px * 3];
    switch(type)
    {
    case "dashed"_h:
        spf::dashed_texture(buffer, size_px);
        break;
    case "grid"_h:
        spf::grid_texture(buffer, size_px);
        break;
    case "checkerboard"_h:
        spf::checkerboard_texture(buffer, size_px);
        break;
    case "white"_h:
        spf::colored_texture(buffer, size_px, 255, 255, 255);
        break;
    case "red"_h:
        spf::colored_texture(buffer, size_px, 255, 0, 0);
        break;
    default:
        spf::checkerboard_texture(buffer, size_px);
        hname = "checkerboard"_h;
        break;
    }

    Texture2DDescriptor descriptor;
    descriptor.width = size_px;
    descriptor.height = size_px;
    descriptor.mips = 0;
    descriptor.data = buffer;
    descriptor.image_format = ImageFormat::RGB8;
    descriptor.flags = TF_MUST_FREE; // Let the renderer free the resources once the texture is loaded

    // Create texture
    TextureHandle tex = Renderer::create_texture_2D(descriptor);
    s_storage.special_textures_cache_.insert({hname, tex});
    return tex;
}

const ComponentPBRMaterial& AssetManager::load_material(const fs::path& file_path)
{
    return s_storage.material_manager.load(file_path);
}

const Mesh& AssetManager::load_mesh(const fs::path& file_path)
{
    return s_storage.mesh_manager.load(file_path);
}

const FreeTexture& AssetManager::load_texture(const fs::path& file_path, std::optional<Texture2DDescriptor> options)
{
    return s_storage.texture_manager.load(file_path, options);
}

const TextureAtlas& AssetManager::load_texture_atlas(const fs::path& file_path)
{
    return s_storage.texture_atlas_manager.load(file_path);
}

const FontAtlas& AssetManager::load_font_atlas(const fs::path& file_path)
{
    return s_storage.font_atlas_manager.load(file_path);
}

const Environment& AssetManager::load_environment(const fs::path& file_path)
{
    return s_storage.environment_manager.load(file_path);
}

void AssetManager::release_material(hash_t hname) { s_storage.material_manager.release(hname); }

void AssetManager::release_mesh(hash_t hname) { s_storage.mesh_manager.release(hname); }

void AssetManager::release_texture(hash_t hname) { s_storage.texture_manager.release(hname); }

void AssetManager::release_texture_atlas(hash_t hname) { s_storage.texture_atlas_manager.release(hname); }

void AssetManager::release_font_atlas(hash_t hname) { s_storage.font_atlas_manager.release(hname); }

void AssetManager::release_environment(hash_t hname) { s_storage.environment_manager.release(hname); }

hash_t AssetManager::load_material_async(const fs::path& file_path)
{
    return s_storage.material_manager.load_async(file_path);
}

hash_t AssetManager::load_mesh_async(const fs::path& file_path)
{
    return s_storage.mesh_manager.load_async(file_path);
}

hash_t AssetManager::load_texture_async(const fs::path& file_path)
{
    return s_storage.texture_manager.load_async(file_path);
}

hash_t AssetManager::load_texture_atlas_async(const fs::path& file_path)
{
    return s_storage.texture_atlas_manager.load_async(file_path);
}

hash_t AssetManager::load_font_atlas_async(const fs::path& file_path)
{
    return s_storage.font_atlas_manager.load_async(file_path);
}

hash_t AssetManager::load_environment_async(const fs::path& file_path)
{
    return s_storage.environment_manager.load_async(file_path);
}

void AssetManager::on_material_ready(hash_t future_res, std::function<void(const ComponentPBRMaterial&)> then)
{
    s_storage.material_manager.on_ready(future_res, then);
}

void AssetManager::on_mesh_ready(hash_t future_res, std::function<void(const Mesh&)> then)
{
    s_storage.mesh_manager.on_ready(future_res, then);
}

void AssetManager::on_texture_ready(hash_t future_res, std::function<void(const FreeTexture&)> then)
{
    s_storage.texture_manager.on_ready(future_res, then);
}

void AssetManager::on_texture_atlas_ready(hash_t future_res, std::function<void(const TextureAtlas&)> then)
{
    s_storage.texture_atlas_manager.on_ready(future_res, then);
}

void AssetManager::on_font_atlas_ready(hash_t future_res, std::function<void(const FontAtlas&)> then)
{
    s_storage.font_atlas_manager.on_ready(future_res, then);
}

void AssetManager::on_environment_ready(hash_t future_res, std::function<void(const Environment&)> then)
{
    s_storage.environment_manager.on_ready(future_res, then);
}

void AssetManager::launch_async_tasks()
{
    // TMP: single thread loading all resources
    std::thread task([&]() {
        s_storage.texture_atlas_manager.async_work();
        s_storage.font_atlas_manager.async_work();
        s_storage.mesh_manager.async_work();
        s_storage.material_manager.async_work();
        s_storage.texture_manager.async_work();
        s_storage.environment_manager.async_work();
    });
    task.detach();
}

void AssetManager::update()
{
    s_storage.texture_atlas_manager.sync_work();
    s_storage.font_atlas_manager.sync_work();
    s_storage.mesh_manager.sync_work();
    s_storage.material_manager.sync_work();
    s_storage.texture_manager.sync_work();
    s_storage.environment_manager.sync_work();
}

/*

template <typename KeyT, typename HandleT>
static void erase_by_value(std::map<KeyT, HandleT>& cache, HandleT handle)
{
    auto it = cache.begin();
    for(; it!=cache.end(); ++it)
        if(it->second == handle)
            break;
    cache.erase(it);
}

void AssetManager::release(ShaderHandle handle)
{
    W_PROFILE_FUNCTION()

    W_ASSERT_FMT(handle.is_valid(), "ShaderHandle of index %hu is invalid.", handle.index);
    DLOGN("asset") << "[AssetManager] Releasing shader:" << std::endl;

    erase_by_value(s_storage.shader_cache_, handle);
    Renderer::destroy(handle);

    DLOG("asset",1) << "handle: " << WCC('v') << handle.index << std::endl;
}

*/

} // namespace erwin