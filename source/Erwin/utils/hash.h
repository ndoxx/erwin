#pragma once

#include "glm/glm.hpp"
#include <cstdint>
#include <functional>

namespace erwin
{
namespace wh
{

inline uint32_t hash(float f)
{
    union // Used to reinterpret float mantissa as an unsigned integer...
    {
        float f_;
        uint32_t u_;
    };
    f_ = f;

    // ... with a 3 LSB epsilon (floats whose mantissas are only 3 bits different will share a common hash)
    return u_ & 0xfffffff8; // BEWARE: Depends on endianness
}
inline uint32_t hash(uint32_t x)
{
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}
inline uint32_t unhash(uint32_t x)
{
    x = ((x >> 16) ^ x) * 0x119de1f3;
    x = ((x >> 16) ^ x) * 0x119de1f3;
    x = (x >> 16) ^ x;
    return x;
}
inline uint64_t hash(uint64_t x)
{
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}
inline uint64_t unhash(uint64_t x)
{
    x = (x ^ (x >> 31) ^ (x >> 62)) * UINT64_C(0x319642b2d24d8ec3);
    x = (x ^ (x >> 27) ^ (x >> 54)) * UINT64_C(0x96de1b173f119089);
    x = x ^ (x >> 30) ^ (x >> 60);
    return x;
}

struct pair_hash
{
    template <typename T1, typename T2> std::size_t operator()(const std::pair<T1, T2>& pair) const
    {
        return std::hash<T1>()(pair.first) ^ std::hash<T1>()(pair.second);
    }
};

struct vec3_hash
{
    std::size_t operator()(const glm::vec3& vec) const
    {
        std::size_t seed = 0;

        // Combine component hashes to obtain a position hash
        // Similar to Boost's hash_combine function
        for(int ii = 0; ii < 3; ++ii)
        {
            seed ^= hash(vec[ii]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        return seed; // Two epsilon-distant vertices will share a common hash
    }
};

} // namespace wh
} // namespace erwin