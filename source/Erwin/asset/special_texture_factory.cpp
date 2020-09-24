#include "asset/special_texture_factory.h"

namespace erwin
{
namespace spf
{

constexpr uint8_t k_dark  = 153;
constexpr uint8_t k_light = 255;

void dashed_texture(uint8_t* buffer, uint32_t size_px)
{
	for(uint32_t yy=0; yy<size_px; ++yy)
	{
		for(uint32_t xx=0; xx<size_px; ++xx)
		{
			uint32_t index = 3*(yy*size_px+xx);
			bool step = (xx+yy)%16 < 8;
			buffer[index+0] = step ? k_light : k_dark;
			buffer[index+1] = step ? k_light : k_dark;
			buffer[index+2] = step ? k_light : k_dark;
		}
	}
}

void grid_texture(uint8_t* buffer, uint32_t size_px)
{
	for(uint32_t yy=0; yy<size_px; ++yy)
	{
		for(uint32_t xx=0; xx<size_px; ++xx)
		{
			uint32_t index = 3*(yy*size_px+xx);
			bool step = (xx%16 < 8) && (yy%16 < 8);
			buffer[index+0] = step ? k_light : k_dark;
			buffer[index+1] = step ? k_light : k_dark;
			buffer[index+2] = step ? k_light : k_dark;
		}
	}
}

void checkerboard_texture(uint8_t* buffer, uint32_t size_px)
{
	for(uint32_t yy=0; yy<size_px; ++yy)
	{
		for(uint32_t xx=0; xx<size_px; ++xx)
		{
			uint32_t index = 3*(yy*size_px+xx);
			bool step = ((xx%16 < 8) && (yy%16 < 8)) || ((xx%16 > 8) && (yy%16 > 8));
			buffer[index+0] = step ? k_light : k_dark;
			buffer[index+1] = step ? k_light : k_dark;
			buffer[index+2] = step ? k_light : k_dark;
		}
	}
}

void colored_texture(uint8_t* buffer, uint32_t size_px, uint8_t r, uint8_t g, uint8_t b)
{
	for(uint32_t yy=0; yy<size_px; ++yy)
	{
		for(uint32_t xx=0; xx<size_px; ++xx)
		{
			uint32_t index = 3*(yy*size_px+xx);
			buffer[index+0] = r;
			buffer[index+1] = g;
			buffer[index+2] = b;
		}
	}
}

} // namespace spf
} // namespace erwin