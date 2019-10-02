#pragma once

namespace fudge
{

enum class Compression: uint8_t
{
    None = 0,
    DXT5,
    Deflate
};

}