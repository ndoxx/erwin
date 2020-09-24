#pragma once

#include "utils/sparse_set.hpp"

namespace erwin
{

template <size_t SIZE> using HandlePoolT = SecureSparsePool<uint32_t, SIZE, 16>;

} // namespace erwin