#pragma once

#include "core/core.h"
#include "filesystem/file_path.h"
#include "render/handles.h"
#include "render/texture_common.h"
#include "asset/loader_common.h"
#include <cstdint>
#include <future>
#include <optional>
#include <map>

namespace erwin
{

struct ComponentPBRMaterial;
struct Mesh;
struct TextureAtlas;
struct FontAtlas;
struct Environment;
struct FreeTexture;
class AssetManager
{
public:
    // * Synchronous operations
    // Load a shader from file, by default, name is extracted from file path
    static ShaderHandle load_shader(const fs::path& file_path, const std::string& name = "");
    // Create (or get) a UBO to be associated to a material data type
    template <typename ComponentT> static inline UniformBufferHandle create_material_data_buffer()
    {
        using MaterialData = typename ComponentT::MaterialData;
        return create_material_data_buffer(ctti::type_id<ComponentT>().hash(), sizeof(MaterialData));
    }
    // Proceduraly generated assets for the editor and debug purposes
    // type can be "dashed"_h, "grid"_h, "checkerboard"_h, "white"_h or "red"_h
    static TextureHandle create_debug_texture(hash_t type, uint32_t size_px);

    // Load any resource
    template <typename ResT> static const ResT& load(size_t reg, const FilePath& file_path);
    template <typename ResT, typename OptT> static const ResT& load(size_t reg, const FilePath& file_path, const OptT& options);
    // Release any resource
    template <typename ResT> static void release(size_t reg, hash_t hname);

    // * Asynchronous operations
    // Load any resource by type
    static hash_t load_resource_async(size_t reg, AssetMetaData::AssetType type, const FilePath& file_path);
    // Load any resource asynchronously
    template <typename ResT> static hash_t load_async(size_t reg, const FilePath& file_path);
    // Execute a callback when a resource is ready
    template <typename ResT> static void on_ready(hash_t future_res, std::function<void(const ResT&)> then);

    // Execute async tasks independently
    static void launch_async_tasks();
    // Execute synchronous tasks if any
    static void update();

    // Get resource paths table
    static const std::map<hash_t, AssetMetaData>& get_resource_meta(size_t reg);
    // Asset registry creation / destruction
    static size_t create_asset_registry();
    static void release_registry(size_t reg);

private:
    static UniformBufferHandle create_material_data_buffer(uint64_t id, uint32_t size);
};


// Specializations
// Load a material from a TOM file (or get from cache) and return a ref to an internally stored material component
template<> const ComponentPBRMaterial& AssetManager::load<ComponentPBRMaterial>(size_t reg, const FilePath& file_path);
// Load a mesh from a WESH file (or get from cache)
template<> const Mesh& AssetManager::load<Mesh>(size_t reg, const FilePath& file_path);
// Load an image from file (or get from cache) and return a render handle to a texture created from image data
template<> const FreeTexture& AssetManager::load<FreeTexture>(size_t reg, const FilePath& file_path);
// Same as before, but enforce some texture properties via a descriptor
template<> const FreeTexture& AssetManager::load<FreeTexture,Texture2DDescriptor>(size_t reg, const FilePath& file_path, const Texture2DDescriptor& options);
// Load a font as a texture atlas from file (or get from cache)
template<> const TextureAtlas& AssetManager::load<TextureAtlas>(size_t reg, const FilePath& file_path);
// Load a font as a texture atlas from file (or get from cache)
template<> const FontAtlas& AssetManager::load<FontAtlas>(size_t reg, const FilePath& file_path);
// Load an environment map from file (or get from cache)
template<> const Environment& AssetManager::load<Environment>(size_t reg, const FilePath& file_path);

// Free GPU resources associated to a material and remove from cache
template<> void AssetManager::release<ComponentPBRMaterial>(size_t reg, hash_t hname);
// Free GPU resources associated to a mesh
template<> void AssetManager::release<Mesh>(size_t reg, hash_t hname);
// Free GPU resource associated to texture
template<> void AssetManager::release<FreeTexture>(size_t reg, hash_t hname);
// Free GPU resource associated to texture atlas
template<> void AssetManager::release<TextureAtlas>(size_t reg, hash_t hname);
// Free GPU resource associated to font atlas
template<> void AssetManager::release<FontAtlas>(size_t reg, hash_t hname);
// Free GPU resources associated to environment
template<> void AssetManager::release<Environment>(size_t reg, hash_t hname);

// Generate an async material loading task if material not in cache, return path string hash as a token
template<> hash_t AssetManager::load_async<ComponentPBRMaterial>(size_t reg, const FilePath& file_path);
// Generate an async mesh loading task if mesh not in cache, return path string hash as a token
template<> hash_t AssetManager::load_async<Mesh>(size_t reg, const FilePath& file_path);
// Generate an async texture loading task if texture not in cache, return path string hash as a token
template<> hash_t AssetManager::load_async<FreeTexture>(size_t reg, const FilePath& file_path);
// Generate an async texture atlas loading task if not in cache, return path string hash as a token
template<> hash_t AssetManager::load_async<TextureAtlas>(size_t reg, const FilePath& file_path);
// Generate an async font atlas loading task if not in cache, return path string hash as a token
template<> hash_t AssetManager::load_async<FontAtlas>(size_t reg, const FilePath& file_path);
// Generate an async environment loading task if environment not in cache, return path string hash as a token
template<> hash_t AssetManager::load_async<Environment>(size_t reg, const FilePath& file_path);

// Execute a callback when a material is ready
template<> void AssetManager::on_ready<ComponentPBRMaterial>(hash_t future_res, std::function<void(const ComponentPBRMaterial&)> then);
// Execute a callback when a mesh is ready
template<> void AssetManager::on_ready<Mesh>(hash_t future_res, std::function<void(const Mesh&)> then);
// Execute a callback when a texture is ready
template<> void AssetManager::on_ready<FreeTexture>(hash_t future_res, std::function<void(const FreeTexture&)> then);
// Execute a callback when a font atlas is ready
template<> void AssetManager::on_ready<TextureAtlas>(hash_t future_res, std::function<void(const TextureAtlas&)> then);
// Execute a callback when a font atlas is ready
template<> void AssetManager::on_ready<FontAtlas>(hash_t future_res, std::function<void(const FontAtlas&)> then);
// Execute a callback when a font atlas is ready
template<> void AssetManager::on_ready<Environment>(hash_t future_res, std::function<void(const Environment&)> then);

} // namespace erwin