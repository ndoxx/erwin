#pragma once

#include <filesystem>

#include "common.h"

namespace fs = std::filesystem;

namespace fudge
{
namespace texmap
{

// Configure texture map sepcifications
extern bool configure(const fs::path& filepath);
// Create a tom file from texture maps found in input directory.
extern void make_tom(const fs::path& input_dir, const fs::path& output_dir);

} // namespace texmap
} // namespace fudge