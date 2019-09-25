#pragma once

#include <cstdint>

namespace fudge
{

extern void compress_dxt_5(uint8_t* in_buf, uint8_t*& out_buf, uint32_t width, uint32_t height);


} // namespace fudge