#pragma once

#include "core/handle.h"

namespace erwin
{

constexpr std::size_t k_max_asset_handles = 128;
constexpr std::size_t k_max_atlases = 128;
constexpr std::size_t k_max_texture_groups = 128;
constexpr std::size_t k_max_material_layouts = 128;

HANDLE_DECLARATION( TextureAtlasHandle );
HANDLE_DECLARATION( TextureGroupHandle );
HANDLE_DECLARATION( MaterialLayoutHandle );


} // namespace erwin