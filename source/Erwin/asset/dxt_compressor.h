#pragma once
#include <cstdint>

namespace erwin
{
namespace dxt
{

extern uint8_t* compress_DXT5(uint8_t* in_buf, uint32_t width, uint32_t height);

} // namespace dxt
} // namespace erwin