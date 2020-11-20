#pragma once

#include <kibble/util/sparse_set.h>

namespace erwin
{

template <size_t SIZE> using HandlePoolT = kb::SecureSparsePool<uint32_t, SIZE, 16>;

} // namespace erwin