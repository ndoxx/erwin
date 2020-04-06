#pragma once

#include "core/handle.h"

namespace erwin
{

constexpr std::size_t k_max_atlases = 128;
constexpr std::size_t k_max_font_atlases = 8;
constexpr std::size_t k_max_texture_groups = 128;
constexpr std::size_t k_max_materials = 512;

HANDLE_DECLARATION( TextureAtlasHandle );
HANDLE_DECLARATION( FontAtlasHandle );
HANDLE_DECLARATION( TextureGroupHandle );
HANDLE_DECLARATION( MaterialHandle );


} // namespace erwin