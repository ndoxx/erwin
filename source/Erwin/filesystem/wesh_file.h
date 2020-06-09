#pragma once

/*
    erWin mESH format for geometry storage
*/

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace erwin
{
namespace wesh
{

struct WeshDescriptor
{
    std::vector<float> vertex_data;
    std::vector<uint32_t> index_data;
};


WeshDescriptor read(const fs::path& path);


} // namespace wesh
} // namespace erwin