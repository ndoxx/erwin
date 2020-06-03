#pragma once

#include <filesystem>
#include <cstdint>
#include <future>
#include "render/texture_common.h"

namespace fs = std::filesystem;

namespace erwin
{

struct ComponentPBRMaterial;
class AssetManagerE
{
public:
	static uint64_t load_material_async(const fs::path& file_path);
    static void on_material_ready(uint64_t future_mat, std::function<void(const ComponentPBRMaterial&)> then);

    static void launch_async_tasks();
    static void update();
};


} // namespace erwin