#pragma once

#include <cstdint>
#include "imgui.h"

namespace erwin
{

inline ImVec4 imgui_rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a=255)
{
	return {float(r)/255.f, float(g)/255.f, float(b)/255.f, float(a)/255.f};
}


} // namespace erwin