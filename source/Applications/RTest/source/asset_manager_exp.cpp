#include "asset_manager_exp.h"
#include "core/intern_string.h"
#include "entity/component_PBR_material.h"
#include "render/renderer.h"
#include "utils/future.hpp"

#include "material_loader.h"
#include "texture_loader.h"
#include "resource_manager.hpp"

#include <chrono>
#include <thread>

namespace erwin
{
namespace experimental
{

static struct
{
    ResourceManager<MaterialLoader> material_manager;
    ResourceManager<TextureLoader> texture_manager;
    // TextureManager texture_manager;

    std::map<hash_t, ShaderHandle> shader_cache;
    std::map<uint64_t, UniformBufferHandle> ubo_cache;
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

const ComponentPBRMaterial& AssetManager::load_material(const fs::path& file_path)
{
    return s_storage.material_manager.load(file_path);
}

void AssetManager::release_material(hash_t hname) { s_storage.material_manager.release(hname); }

std::pair<TextureHandle, Texture2DDescriptor> AssetManager::load_texture(const fs::path& file_path)
{
    return s_storage.texture_manager.load(file_path);
}

hash_t AssetManager::load_material_async(const fs::path& file_path)
{
    return s_storage.material_manager.load_async(file_path);
}

void AssetManager::on_material_ready(hash_t future_mat, std::function<void(const ComponentPBRMaterial&)> then)
{
    s_storage.material_manager.on_ready(future_mat, then);
}

hash_t AssetManager::load_texture_async(const fs::path& file_path)
{
    return s_storage.texture_manager.load_async(file_path);
}

void AssetManager::on_texture_ready(hash_t future_texture,
                                    std::function<void(const std::pair<TextureHandle, Texture2DDescriptor>&)> then)
{
    s_storage.texture_manager.on_ready(future_texture, then);
}

void AssetManager::launch_async_tasks()
{
    s_storage.texture_manager.launch_async_tasks();
    s_storage.material_manager.launch_async_tasks();
}

void AssetManager::update()
{
    s_storage.texture_manager.update();
    s_storage.material_manager.update();
}

} // namespace experimental
} // namespace erwin