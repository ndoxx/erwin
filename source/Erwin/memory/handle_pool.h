#pragma once

#include "utils/sparse_set.hpp"

namespace erwin
{

template <size_t SIZE> using HandlePoolT = SparsePool<uint16_t, SIZE>;

} // namespace erwin