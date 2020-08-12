#include "asset/asset_manager.h"
#include "core/intern_string.h"
#include "entity/component/PBR_material.h"
#include "render/renderer.h"
#include "utils/future.hpp"
#include "filesystem/filesystem.h"

#include "asset/atlas_loader.h"
#include "asset/environment_loader.h"
#include "asset/material_loader.h"
#include "asset/mesh_loader.h"
#include "asset/resource_cache.hpp"
#include "asset/special_texture_factory.h"
#include "asset/texture_loader.h"

#include "utils/sparse_set.hpp"

#include <chrono>
#include <thread>

namespace erwin
{

static constexpr size_t k_max_asset_registries = 8;

struct AssetRegistry
{
    std::map<hash_t, AssetMetaData> asset_meta_;

    inline void insert(const AssetMetaData& meta_data)
    {
        hash_t hname = meta_data.file_path.resource_id();
        asset_meta_[hname] = meta_data;
    }

    inline void erase(hash_t hname) { asset_meta_.erase(hname); }

    inline bool has(hash_t hname) { return (asset_meta_.find(hname) != asset_meta_.end()); }

    inline void clear() { asset_meta_.clear(); }
};

static struct
{
    ResourceCache<MaterialLoader> material_cache;
    ResourceCache<TextureLoader> texture_cache;
    ResourceCache<FontAtlasLoader> font_atlas_cache;
    ResourceCache<TextureAtlasLoader> texture_atlas_cache;
    ResourceCache<EnvironmentLoader> environment_cache;
    ResourceCache<MeshLoader> mesh_cache;

    SparsePool<size_t, k_max_asset_registries> registry_handle_pool;
    std::array<AssetRegistry, k_max_asset_registries> registry;

    std::map<hash_t, ShaderHandle> shader_cache;
    std::map<uint64_t, UniformBufferHandle> ubo_cache;
    std::map<hash_t, TextureHandle> special_textures_cache_;

} s_storage;

template <typename ManagerT>
static void release_if_not_shared(size_t reg, hash_t handle, ManagerT& manager)
{
    bool shared = false;
    for(auto it = s_storage.registry_handle_pool.begin(); it != s_storage.registry_handle_pool.end(); ++it)
    {
        auto oreg = *it;
        if(oreg!=reg && s_storage.registry[oreg].has(handle))
        {
            shared = true;
            break;
        }
    }

    if(!shared)
        manager.release(handle);
}

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

    hash_t hname = H_(file_path.c_str());
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
    DLOG("asset", 1) << "ShaderHandle: " << WCC('v') << handle.index() << std::endl;
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

template<> const ComponentPBRMaterial& AssetManager::load<ComponentPBRMaterial>(size_t reg, const WPath& file_path)
{
    auto&& [res, meta] = s_storage.material_cache.load(file_path);
    s_storage.registry[reg].insert(meta);
    return res;
}

template<> const Mesh& AssetManager::load<Mesh>(size_t reg, const WPath& file_path)
{
    auto&& [res, meta] = s_storage.mesh_cache.load(file_path);
    s_storage.registry[reg].insert(meta);
    return res;
}

template<> const FreeTexture& AssetManager::load<FreeTexture>(size_t reg, const WPath& file_path)
{
    auto&& [res, meta] = s_storage.texture_cache.load(file_path);
    s_storage.registry[reg].insert(meta);
    return res;
}

template<> const FreeTexture& AssetManager::load<FreeTexture,Texture2DDescriptor>(size_t reg, const WPath& file_path, const Texture2DDescriptor& options)
{
    auto&& [res, meta] = s_storage.texture_cache.load(file_path, options);
    s_storage.registry[reg].insert(meta);
    return res;
}

template<> const TextureAtlas& AssetManager::load<TextureAtlas>(size_t reg, const WPath& file_path)
{
    auto&& [res, meta] = s_storage.texture_atlas_cache.load(file_path);
    s_storage.registry[reg].insert(meta);
    return res;
}

template<> const FontAtlas& AssetManager::load<FontAtlas>(size_t reg, const WPath& file_path)
{
    auto&& [res, meta] = s_storage.font_atlas_cache.load(file_path);
    s_storage.registry[reg].insert(meta);
    return res;
}

template<> const Environment& AssetManager::load<Environment>(size_t reg, const WPath& file_path)
{
    auto&& [res, meta] = s_storage.environment_cache.load(file_path);
    s_storage.registry[reg].insert(meta);
    return res;
}

template<> void AssetManager::release<ComponentPBRMaterial>(size_t reg, hash_t hname)
{
    release_if_not_shared(reg, hname, s_storage.material_cache);
    s_storage.registry[reg].erase(hname);
}

template<> void AssetManager::release<Mesh>(size_t reg, hash_t hname)
{
    release_if_not_shared(reg, hname, s_storage.mesh_cache);
    s_storage.registry[reg].erase(hname);
}

template<> void AssetManager::release<FreeTexture>(size_t reg, hash_t hname)
{
    release_if_not_shared(reg, hname, s_storage.texture_cache);
    s_storage.registry[reg].erase(hname);
}

template<> void AssetManager::release<TextureAtlas>(size_t reg, hash_t hname)
{
    release_if_not_shared(reg, hname, s_storage.texture_atlas_cache);
    s_storage.registry[reg].erase(hname);
}

template<> void AssetManager::release<FontAtlas>(size_t reg, hash_t hname)
{
    release_if_not_shared(reg, hname, s_storage.font_atlas_cache);
    s_storage.registry[reg].erase(hname);
}

template<> void AssetManager::release<Environment>(size_t reg, hash_t hname)
{
    release_if_not_shared(reg, hname, s_storage.environment_cache);
    s_storage.registry[reg].erase(hname);
}

template<> hash_t AssetManager::load_async<ComponentPBRMaterial>(size_t reg, const WPath& file_path)
{
    auto&& [handle, meta] = s_storage.material_cache.load_async(file_path);
    s_storage.registry[reg].insert(meta);
    return handle;
}

template<> hash_t AssetManager::load_async<Mesh>(size_t reg, const WPath& file_path)
{
    auto&& [handle, meta] = s_storage.mesh_cache.load_async(file_path);
    s_storage.registry[reg].insert(meta);
    return handle;
}

template<> hash_t AssetManager::load_async<FreeTexture>(size_t reg, const WPath& file_path)
{
    auto&& [handle, meta] = s_storage.texture_cache.load_async(file_path);
    s_storage.registry[reg].insert(meta);
    return handle;
}

template<> hash_t AssetManager::load_async<TextureAtlas>(size_t reg, const WPath& file_path)
{
    auto&& [handle, meta] = s_storage.texture_atlas_cache.load_async(file_path);
    s_storage.registry[reg].insert(meta);
    return handle;
}

template<> hash_t AssetManager::load_async<FontAtlas>(size_t reg, const WPath& file_path)
{
    auto&& [handle, meta] = s_storage.font_atlas_cache.load_async(file_path);
    s_storage.registry[reg].insert(meta);
    return handle;
}

template<> hash_t AssetManager::load_async<Environment>(size_t reg, const WPath& file_path)
{
    auto&& [handle, meta] = s_storage.environment_cache.load_async(file_path);
    s_storage.registry[reg].insert(meta);
    return handle;
}

hash_t AssetManager::load_resource_async(size_t reg, AssetMetaData::AssetType type, const WPath& file_path)
{
    switch(type)
    {
        case AssetMetaData::AssetType::ImageFilePNG:    return load_async<FreeTexture>(reg, file_path);
        case AssetMetaData::AssetType::ImageFileHDR:    return load_async<FreeTexture>(reg, file_path);
        case AssetMetaData::AssetType::EnvironmentHDR:  return load_async<Environment>(reg, file_path);
        case AssetMetaData::AssetType::MaterialTOM:     return load_async<ComponentPBRMaterial>(reg, file_path);
        case AssetMetaData::AssetType::TextureAtlasCAT: return load_async<TextureAtlas>(reg, file_path);
        case AssetMetaData::AssetType::FontAtlasCAT:    return load_async<FontAtlas>(reg, file_path);
        case AssetMetaData::AssetType::MeshWESH:        return load_async<Mesh>(reg, file_path);
        default: return 0;
    }
}

template<> void AssetManager::on_ready<ComponentPBRMaterial>(hash_t future_res, std::function<void(const ComponentPBRMaterial&)> then)
{
    s_storage.material_cache.on_ready(future_res, then);
}

template<> void AssetManager::on_ready<Mesh>(hash_t future_res, std::function<void(const Mesh&)> then)
{
    s_storage.mesh_cache.on_ready(future_res, then);
}

template<> void AssetManager::on_ready<FreeTexture>(hash_t future_res, std::function<void(const FreeTexture&)> then)
{
    s_storage.texture_cache.on_ready(future_res, then);
}

template<> void AssetManager::on_ready<TextureAtlas>(hash_t future_res, std::function<void(const TextureAtlas&)> then)
{
    s_storage.texture_atlas_cache.on_ready(future_res, then);
}

template<> void AssetManager::on_ready<FontAtlas>(hash_t future_res, std::function<void(const FontAtlas&)> then)
{
    s_storage.font_atlas_cache.on_ready(future_res, then);
}

template<> void AssetManager::on_ready<Environment>(hash_t future_res, std::function<void(const Environment&)> then)
{
    s_storage.environment_cache.on_ready(future_res, then);
}

void AssetManager::launch_async_tasks()
{
    // TMP: single thread loading all resources
    std::thread task([&]() {
        s_storage.environment_cache.async_work();
        s_storage.material_cache.async_work();
        s_storage.mesh_cache.async_work();
        s_storage.texture_atlas_cache.async_work();
        s_storage.font_atlas_cache.async_work();
        s_storage.texture_cache.async_work();
    });
    task.detach();
}

void AssetManager::update()
{
    s_storage.environment_cache.sync_work();
    s_storage.mesh_cache.sync_work();
    s_storage.material_cache.sync_work();
    s_storage.texture_atlas_cache.sync_work();
    s_storage.font_atlas_cache.sync_work();
    s_storage.texture_cache.sync_work();
}

const std::map<hash_t, AssetMetaData>& AssetManager::get_resource_meta(size_t reg)
{
    return s_storage.registry[reg].asset_meta_;
}

size_t AssetManager::create_asset_registry()
{
    return s_storage.registry_handle_pool.acquire();
}

static void release_by_type(size_t reg, hash_t hname, AssetMetaData::AssetType type)
{
    switch(type)
    {
        case AssetMetaData::AssetType::ImageFilePNG:     release_if_not_shared(reg, hname, s_storage.texture_cache); break;
        case AssetMetaData::AssetType::ImageFileHDR:     release_if_not_shared(reg, hname, s_storage.texture_cache); break;
        case AssetMetaData::AssetType::EnvironmentHDR:   release_if_not_shared(reg, hname, s_storage.environment_cache); break;
        case AssetMetaData::AssetType::MaterialTOM:      release_if_not_shared(reg, hname, s_storage.material_cache); break;
        case AssetMetaData::AssetType::TextureAtlasCAT:  release_if_not_shared(reg, hname, s_storage.texture_atlas_cache); break;
        case AssetMetaData::AssetType::FontAtlasCAT:     release_if_not_shared(reg, hname, s_storage.font_atlas_cache); break;
        case AssetMetaData::AssetType::MeshWESH:         release_if_not_shared(reg, hname, s_storage.mesh_cache); break;
        default: break;
    }
}

void AssetManager::release_registry(size_t reg)
{
    for(auto&& [hname, meta]: s_storage.registry[reg].asset_meta_)
        release_by_type(reg, hname, meta.type);

    s_storage.registry[reg].clear();
    s_storage.registry_handle_pool.release(reg);
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