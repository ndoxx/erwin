#pragma once

#include "core/core.h"
#include "filesystem/wpath.h"
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

/**
 * @brief      This static class can load any asset synchronously or
 *             asynchronously. All assets are cached.
 *
 *             All methods of this class are templated by resource type.
 *             Possible types are: ComponentPBRMaterial, Mesh, FreeTexture,
 *             FreeTexture, TextureAtlas, FontAtlas, Environment
 */
class AssetManager
{
public:
    // * Synchronous operations Load a shader from file, by default, name is
    //   extracted from file path

    /**
     * @brief      Load synchronously a GLSL/SPV shader by file path.
     *
     * @param[in]  file_path  Relative file path. Shader is first looked for in
     *                        the system assets folder then in user assets.
     * @param[in]  name       The name that will be used to refer to the shader
     *                        later on.
     *
     * @return     Renderer shader handle
     */
    static ShaderHandle load_shader(const fs::path& file_path, const std::string& name = "");

    /**
     * @brief      Create (or get) a UBO to be associated to a material data
     *             type.
     *
     *             UBO for a given type will be created once and cached for the
     *             lifetime of the application.
     *
     * @tparam     ComponentT  Concrete material component type holding a
     *                         MaterialData structure used as an UBO layout.
     *
     * @return     Handle to the newly created uniform buffer.
     */
    template <typename ComponentT> static inline UniformBufferHandle create_material_data_buffer()
    {
        using MaterialData = typename ComponentT::MaterialData;
        return create_material_data_buffer(ctti::type_id<ComponentT>().hash(), sizeof(MaterialData));
    }

    /**
     * @brief      Proceduraly generate a special "pattern" texture for the editor
     *             and debug purposes.
     *
     * @param[in]  type     Anything from: "dashed"_h, "grid"_h,
     *                      "checkerboard"_h, "white"_h or "red"_h
     * @param[in]  size_px  The resulting square texture size in pixels.
     *
     * @return     The resulting texture handle returned by the renderer.
     */
    static TextureHandle create_debug_texture(hash_t type, uint32_t size_px);

    /**
     * @brief      Load any resource synchronously or get it from cache.
     *
     * @param[in]  reg        Handle to an asset register the resource will be
     *                        held into.
     * @param[in]  file_path  File path to the resource.
     *
     * @tparam     ResT       Concrete resource type (see class description for
     *                        available types).
     *
     * @return     const reference to the newly created (or cached) resource.
     */
    template <typename ResT> static const ResT& load(size_t reg, const WPath& file_path);


    /**
     * @brief      Load a resource synchronously or get it from cache, but pass
     *             loading options as well.
     *
     *             This function is implemented for FreeTexture only with
     *             Texture2DDescriptor as the input options type.
     *
     * @param[in]  reg        Handle to an asset register the resource will be
     *                        held into.
     * @param[in]  file_path  File path to the resource.
     * @param[in]  options    Loading options.
     *
     * @tparam     ResT       Concrete resource type (see class description for
     *                        available types).
     * @tparam     OptT       Concrete type of the loading options structure /
     *                        class.
     *
     * @return     const reference to the newly created (or cached) resource.
     */
    template <typename ResT, typename OptT> static const ResT& load(size_t reg, const WPath& file_path, const OptT& options);
    
    // Release any resource


    /**
     * @brief      Release a resource from GPU memory.
     * 
     *             Synchronous operation.
     *
     * @param[in]  reg    Handle to an asset register the resource is held into.
     * @param[in]  hname  Name of the resource (hash of the relative file path).
     *
     * @tparam     ResT   Type of resource to free.
     */
    template <typename ResT> static void release(size_t reg, hash_t hname);


    // * Asynchronous operations

    /**
     * @brief      Create an asynchronous loading task for a resource.
     *
     *             Resource type is specified by an enumerated type.
     *             Loading tasks will be executed on a slave thread once the
     *             launch_async_tasks() method is called.
     *
     * @param[in]  reg        Handle to an asset register the resource will be
     *                        held into.
     * @param[in]  type       Resource enumerated type.
     * @param[in]  file_path  The file path.
     *
     * @return     String hash of the relative file path to be used as a handle
     *             to use the resource when it is ready.
     */
    static hash_t load_resource_async(size_t reg, AssetMetaData::AssetType type, const WPath& file_path);
    
    /**
     * @brief      Create an asynchronous loading task for a resource.
     *
     *             Resource type is specified by a template parameter.
     *             Loading tasks will be executed on a slave thread once the
     *             launch_async_tasks() method is called.
     *
     * @param[in]  reg        Handle to an asset register the resource will be
     *                        held into.
     * @param[in]  file_path  The file path.
     *
     * @tparam     ResT       Resource concrete type.
     *
     * @return     String hash of the relative file path to be used as a handle
     *             to use the resource when it is ready.
     */
    template <typename ResT> static hash_t load_async(size_t reg, const WPath& file_path);

    /**
     * @brief      Register a callback to be executed when a specified resource
     *             has been loaded by the loader thread.
     *
     * @param[in]  future_res  The future resource handle.
     * @param[in]  then        The callback.
     *
     * @tparam     ResT        Concrete resource type.
     */
    template <typename ResT> static void on_ready(hash_t future_res, std::function<void(const ResT&)> then);

    /**
     * @brief      Create a slave loader thread that will run all registered
     *             loading tasks.
     */
    static void launch_async_tasks();


    /**
     * @brief      Execute callbacks registered via on_ready() if any. 
     */
    static void update();

    /**
     * @brief      Get the asset meta-data map.
     *
     * @param[in]  reg   The asset register.
     *
     * @return     Meta-data map.
     */
    static const std::map<hash_t, AssetMetaData>& get_resource_meta(size_t reg);
    
    // Asset registry creation / destruction
    

    /**
     * @brief      Create an asset registry to hold resource.
     *
     * @return     Handle to the newly created asset registry.
     */
    static size_t create_asset_registry();
    

    /**
     * @brief      Free an asset registry.
     *
     *             All assets in this registry that are not referenced by
     *             another asset registry will be freed as well.
     *
     * @param[in]  reg   Handle to the asset register to be freed.
     */
    static void release_registry(size_t reg);

private:
    static UniformBufferHandle create_material_data_buffer(uint64_t id, uint32_t size);
};


// Specializations
// Load a material from a TOM file (or get from cache) and return a ref to an internally stored material component
template<> const ComponentPBRMaterial& AssetManager::load<ComponentPBRMaterial>(size_t reg, const WPath& file_path);
// Load a mesh from a WESH file (or get from cache)
template<> const Mesh& AssetManager::load<Mesh>(size_t reg, const WPath& file_path);
// Load an image from file (or get from cache) and return a render handle to a texture created from image data
template<> const FreeTexture& AssetManager::load<FreeTexture>(size_t reg, const WPath& file_path);
// Same as before, but enforce some texture properties via a descriptor
template<> const FreeTexture& AssetManager::load<FreeTexture,Texture2DDescriptor>(size_t reg, const WPath& file_path, const Texture2DDescriptor& options);
// Load a font as a texture atlas from file (or get from cache)
template<> const TextureAtlas& AssetManager::load<TextureAtlas>(size_t reg, const WPath& file_path);
// Load a font as a texture atlas from file (or get from cache)
template<> const FontAtlas& AssetManager::load<FontAtlas>(size_t reg, const WPath& file_path);
// Load an environment map from file (or get from cache)
template<> const Environment& AssetManager::load<Environment>(size_t reg, const WPath& file_path);

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
template<> hash_t AssetManager::load_async<ComponentPBRMaterial>(size_t reg, const WPath& file_path);
// Generate an async mesh loading task if mesh not in cache, return path string hash as a token
template<> hash_t AssetManager::load_async<Mesh>(size_t reg, const WPath& file_path);
// Generate an async texture loading task if texture not in cache, return path string hash as a token
template<> hash_t AssetManager::load_async<FreeTexture>(size_t reg, const WPath& file_path);
// Generate an async texture atlas loading task if not in cache, return path string hash as a token
template<> hash_t AssetManager::load_async<TextureAtlas>(size_t reg, const WPath& file_path);
// Generate an async font atlas loading task if not in cache, return path string hash as a token
template<> hash_t AssetManager::load_async<FontAtlas>(size_t reg, const WPath& file_path);
// Generate an async environment loading task if environment not in cache, return path string hash as a token
template<> hash_t AssetManager::load_async<Environment>(size_t reg, const WPath& file_path);

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