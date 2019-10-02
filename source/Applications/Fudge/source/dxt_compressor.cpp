#include <cstring>

#include "dxt_compressor.h"
#include "stb/stb_dxt.h"

namespace fudge
{

inline void extract_block(const uint8_t* in_ptr, int width, uint8_t* colorBlock)
{
    for(int j=0; j<4; ++j)
    {
        memcpy(&colorBlock[j*4*4], in_ptr, 4*4 );
        in_ptr += width * 4;
    }
}

extern uint8_t* compress_dxt_5(uint8_t* in_buf, uint32_t width, uint32_t height)
{
    uint8_t* out_buf = new uint8_t[width*height];
    memset(out_buf, 0, width*height);

    uint8_t block[64];

    uint32_t dst_offset = 0;
    for(int yy=0; yy<height; yy+=4, in_buf+=width*4*4)
    {
        for(int xx=0; xx<width; xx+=4)
        {
            extract_block(in_buf+xx*4, width, block);
            stb_compress_dxt_block(&out_buf[dst_offset], block, 1, STB_DXT_HIGHQUAL);
            dst_offset += 16;
        }
    }

    return out_buf;
}

} // namespace fudge