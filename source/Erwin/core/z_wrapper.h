#pragma once

#include <cstdint>

namespace erwin
{

int get_max_compressed_len(int nLenSrc);

int compress_data(const uint8_t* abSrc, int nLenSrc, uint8_t* abDst, int nLenDst);

int uncompress_data(const uint8_t* abSrc, int nLenSrc, uint8_t* abDst, int nLenDst);


} // namespace erwin