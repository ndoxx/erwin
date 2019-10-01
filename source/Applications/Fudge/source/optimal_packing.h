#pragma once

#include "rectpack2D/src/finders_interface.h"

namespace fudge
{

// Find the optimal packing for a list of rectangles and return the resultant bin size
// Rectangles coordinates (initially set to 0) are modified directly
// max_side: maximal size of a side for the resultant bin
extern rectpack2D::rect_wh pack(std::vector<rectpack2D::rect_xywh>& rectangles, uint32_t max_side=1000);

} // namespace fudge