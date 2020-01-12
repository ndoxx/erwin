#pragma once

#include <cstdint>

namespace erwin
{
namespace math
{

// Previous power of 2 of x
static inline uint32_t pp2(uint32_t x)
{
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x - (x >> 1);
}

// Next power of 2 of x
static inline uint32_t np2(uint32_t x)
{
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return ++x;
}

} // namespace math
} // namespace erwin