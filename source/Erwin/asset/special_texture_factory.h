#pragma once
#include <cstdint>

namespace erwin
{
namespace spf
{

void dashed_texture(uint8_t* buffer, uint32_t size_px);
void grid_texture(uint8_t* buffer, uint32_t size_px);
void checkerboard_texture(uint8_t* buffer, uint32_t size_px);
void colored_texture(uint8_t* buffer, uint32_t size_px, uint8_t r, uint8_t g, uint8_t b);

} // namespace spf
} // namespace erwin