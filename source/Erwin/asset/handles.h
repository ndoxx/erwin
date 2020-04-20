#pragma once

#include "core/handle.h"

namespace erwin
{

constexpr std::size_t k_max_atlases = 128;
constexpr std::size_t k_max_font_atlases = 8;

HANDLE_DECLARATION( TextureAtlasHandle );
HANDLE_DECLARATION( FontAtlasHandle );


} // namespace erwin