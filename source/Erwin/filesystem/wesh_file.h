#pragma once

/*
    erWin mESH format for geometry storage
*/

#include <filesystem>
#include <vector>
#include "asset/bounding.h"

namespace fs = std::filesystem;

namespace erwin
{
namespace wesh
{

struct WeshDescriptor
{
    Extent extent;
    std::vector<float> vertex_data;
    std::vector<uint32_t> index_data;
};


WeshDescriptor read(const fs::path& path);


} // namespace wesh
} // namespace erwin