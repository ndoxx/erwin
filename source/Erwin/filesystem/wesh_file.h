#pragma once

/*
    erWin mESH format for geometry storage
*/

#include <vector>
#include "asset/bounding.h"
#include "filesystem/file_path.h"

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


WeshDescriptor read(const FilePath& path);


} // namespace wesh
} // namespace erwin