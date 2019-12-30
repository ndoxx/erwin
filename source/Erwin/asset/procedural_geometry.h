#pragma once

#include <vector>
#include "render/buffer_layout.h"

namespace erwin
{
namespace pg
{

extern void make_cube(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata, void* params=nullptr);
extern void make_plane(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata, void* params=nullptr);

} // namespace pg
} // namespace erwin