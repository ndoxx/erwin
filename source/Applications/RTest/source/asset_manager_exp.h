#pragma once

#include <filesystem>
#include <cstdint>
#include <future>
#include "core/core.h"
#include "render/handles.h"

namespace fs = std::filesystem;

namespace erwin
{

struct ComponentPBRMaterial;

namespace experimental
{
class AssetManager
{
public:
	// * Synchronous operations
	static ShaderHandle load_shader(const fs::path& filepath, const std::string& name="");
	template <typename ComponentT>
	static inline UniformBufferHandle create_material_data_buffer()
	{
		using MaterialData = typename ComponentT::MaterialData;
		return create_material_data_buffer(ctti::type_id<ComponentT>().hash(), sizeof(MaterialData));
	}
	static const ComponentPBRMaterial& load_material(const fs::path& file_path);
	static void release_material(hash_t hname);

	// * Asynchronous operations
	// Generate an async material loading task if material not in cache, return path string hash as a token
	static hash_t load_material_async(const fs::path& file_path);
	// Execute a callback when a material is ready
    static void on_material_ready(hash_t future_mat, std::function<void(const ComponentPBRMaterial&)> then);

    // Execute async tasks independently
    static void launch_async_tasks();
    // Execute synchronous tasks if any
    static void update();

private:
	static UniformBufferHandle create_material_data_buffer(uint64_t id, uint32_t size);
};

} // namespace experimental
} // namespace erwin