#pragma once

#include "core/handle.h"

namespace erwin
{

constexpr std::size_t k_max_asset_handles = 128;
constexpr std::size_t k_max_atlases = 128;
constexpr std::size_t k_max_materials = 128;

HANDLE_DECLARATION( TextureAtlasHandle );
HANDLE_DECLARATION( MaterialHandle );


} // namespace erwin