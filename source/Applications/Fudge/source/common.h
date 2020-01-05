#pragma once
#include "render/texture_common.h"

namespace fudge
{

enum class BlobCompression: uint8_t
{
    None = 0,
    Deflate
};

}