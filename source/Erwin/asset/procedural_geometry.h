#pragma once

#include <vector>
#include "asset/bounding.h"

namespace erwin
{

class BufferLayout;

namespace pg
{

struct Parameters
{

};

extern Extent make_cube(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata, Parameters* params=nullptr);
extern Extent make_cube_lines(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata, Parameters* params=nullptr);
extern Extent make_plane(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata, Parameters* params=nullptr);
extern Extent make_icosahedron(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata, Parameters* params=nullptr);
extern Extent make_icosphere(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata, Parameters* params=nullptr);
extern Extent make_origin(const BufferLayout& layout, std::vector<float>& vdata, std::vector<uint32_t>& idata, Parameters* params=nullptr);


} // namespace pg
} // namespace erwin