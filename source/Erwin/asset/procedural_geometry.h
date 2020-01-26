#pragma once

#include <vector>
#include "render/buffer_layout.h"
#include "asset/mesh.h"

namespace erwin
{
namespace pg
{

extern Dimensions make_cube(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata, void* params=nullptr);
extern Dimensions make_plane(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata, void* params=nullptr);
extern Dimensions make_icosahedron(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata, void* params=nullptr);

} // namespace pg
} // namespace erwin