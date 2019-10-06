#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace fudge
{
namespace shd
{

extern void make_shader_spirv(const fs::path& source_path, const fs::path& output_dir);

} // namespace shd
} // namespace fudge