#pragma once

#include "core/core.h"
#include "render/handles.h"
#include "render/texture_common.h"
#include <cstdint>
#include <filesystem>
#include <future>

namespace fs = std::filesystem;

namespace erwin
{

struct ComponentPBRMaterial;
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
    // Load a material from a TOM file (or get from cache) and return a ref to an internally stored material component
    static const ComponentPBRMaterial& load_material(const fs::path& file_path);
    // Free GPU resources associated to a material and remove from cache
    static void release_material(hash_t hname);
    // Load an image from file (or get from cache) and return a render handle to a texture created from image data
    static std::pair<TextureHandle, Texture2DDescriptor> load_texture(const fs::path& file_path);
    // Proceduraly generated assets for the editor and debug purposes
    // type can be "dashed"_h, "grid"_h, "checkerboard"_h, "white"_h or "red"_h
    static TextureHandle create_debug_texture(hash_t type, uint32_t size_px);

    // * Asynchronous operations
    // Generate an async material loading task if material not in cache, return path string hash as a token
    static hash_t load_material_async(const fs::path& file_path);
    // Execute a callback when a material is ready
    static void on_material_ready(hash_t future_mat, std::function<void(const ComponentPBRMaterial&)> then);
    // Generate an async texture loading task if texture not in cache, return path string hash as a token
    static hash_t load_texture_async(const fs::path& file_path);
    // Execute a callback when a texture is ready
    static void on_texture_ready(hash_t future_texture,
                                 std::function<void(const std::pair<TextureHandle, Texture2DDescriptor>&)> then);

    // Execute async tasks independently
    static void launch_async_tasks();
    // Execute synchronous tasks if any
    static void update();

private:
    static UniformBufferHandle create_material_data_buffer(uint64_t id, uint32_t size);
};

} // namespace erwin