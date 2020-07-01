#pragma once

#include "core/core.h"
#include "render/handles.h"
#include "render/texture_common.h"
#include "asset/loader_common.h"
#include <cstdint>
#include <filesystem>
#include <future>
#include <optional>
#include <map>

namespace fs = std::filesystem;

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

    // Load a material from a TOM file (or get from cache) and return a ref to an internally stored material component
    static const ComponentPBRMaterial& load_material(size_t reg, const fs::path& file_path);
    // Load a mesh from a WESH file (or get from cache)
    static const Mesh& load_mesh(size_t reg, const fs::path& file_path);
    // Load an image from file (or get from cache) and return a render handle to a texture created from image data
    static const FreeTexture& load_texture(size_t reg, const fs::path& file_path, std::optional<Texture2DDescriptor> options = {});
    // Load a font as a texture atlas from file (or get from cache)
    static const TextureAtlas& load_texture_atlas(size_t reg, const fs::path& file_path);
    // Load a font as a texture atlas from file (or get from cache)
    static const FontAtlas& load_font_atlas(size_t reg, const fs::path& file_path);
    // Load an environment map from file (or get from cache)
    static const Environment& load_environment(size_t reg, const fs::path& file_path);

    // Free GPU resources associated to a material and remove from cache
    static void release_material(size_t reg, hash_t hname);
    // Free GPU resources associated to a mesh
    static void release_mesh(size_t reg, hash_t hname);
    // Free GPU resource associated to texture
    static void release_texture(size_t reg, hash_t hname);
    // Free GPU resource associated to texture atlas
    static void release_texture_atlas(size_t reg, hash_t hname);
    // Free GPU resource associated to font atlas
    static void release_font_atlas(size_t reg, hash_t hname);
    // Free GPU resources associated to environment
    static void release_environment(size_t reg, hash_t hname);

    // * Asynchronous operations
    // Generate an async material loading task if material not in cache, return path string hash as a token
    static hash_t load_material_async(size_t reg, const fs::path& file_path);
    // Generate an async mesh loading task if mesh not in cache, return path string hash as a token
    static hash_t load_mesh_async(size_t reg, const fs::path& file_path);
    // Generate an async texture loading task if texture not in cache, return path string hash as a token
    static hash_t load_texture_async(size_t reg, const fs::path& file_path);
    // Generate an async texture atlas loading task if not in cache, return path string hash as a token
    static hash_t load_texture_atlas_async(size_t reg, const fs::path& file_path);
    // Generate an async font atlas loading task if not in cache, return path string hash as a token
    static hash_t load_font_atlas_async(size_t reg, const fs::path& file_path);
    // Generate an async environment loading task if environment not in cache, return path string hash as a token
    static hash_t load_environment_async(size_t reg, const fs::path& file_path);

    // Execute a callback when a material is ready
    static void on_material_ready(hash_t future_res, std::function<void(const ComponentPBRMaterial&)> then);
    // Execute a callback when a mesh is ready
    static void on_mesh_ready(hash_t future_res, std::function<void(const Mesh&)> then);
    // Execute a callback when a texture is ready
    static void on_texture_ready(hash_t future_res, std::function<void(const FreeTexture&)> then);
    // Execute a callback when a font atlas is ready
    static void on_texture_atlas_ready(hash_t future_res, std::function<void(const TextureAtlas&)> then);
    // Execute a callback when a font atlas is ready
    static void on_font_atlas_ready(hash_t future_res, std::function<void(const FontAtlas&)> then);
    // Execute a callback when a font atlas is ready
    static void on_environment_ready(hash_t future_res, std::function<void(const Environment&)> then);

    // Execute async tasks independently
    static void launch_async_tasks();
    // Execute synchronous tasks if any
    static void update();

    // Get resource paths table
    static const std::map<hash_t, AssetMetaData>& get_resource_meta(size_t reg);

    static size_t create_asset_registry();
    static void release_registry(size_t reg);

private:
    static UniformBufferHandle create_material_data_buffer(uint64_t id, uint32_t size);
};

} // namespace erwin