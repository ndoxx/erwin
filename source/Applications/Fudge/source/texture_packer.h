#pragma once

#include <filesystem>

#include "common.h"

namespace fs = std::filesystem;

namespace fudge
{
namespace texmap
{

// Set lossless compression option
extern void set_compression(Compression compression);
// Create a tom file from texture maps found in input directory.
extern void make_tom(const fs::path& input_dir, const fs::path& output_dir);

} // namespace texmap
} // namespace fudge