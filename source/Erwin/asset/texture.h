#pragma once

#include <cstdint>
#include "render/handles.h"

namespace erwin
{

struct FreeTexture
{
	TextureHandle handle;
	uint32_t width;
	uint32_t height;
};


} // namespace erwin