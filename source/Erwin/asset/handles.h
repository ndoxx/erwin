#pragma once

#include "core/handle.h"

namespace erwin
{

constexpr std::size_t k_max_atlases = 128;
constexpr std::size_t k_max_font_atlases = 8;

HANDLE_DECLARATION( TextureAtlasHandle, k_max_atlases );
HANDLE_DECLARATION( FontAtlasHandle, k_max_font_atlases );


} // namespace erwin