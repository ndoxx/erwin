#pragma once

#include <array>
#include "filesystem/filesystem.h"
#include "filesystem/tom_file.h"
#include "render/handles.h"

namespace erwin
{

static constexpr std::size_t k_max_texture_slots = 8; 

// Enforces a strict texture slot order inside a material's texture array
// TODO: will also be used to check shader compatibility
struct MaterialLayout
{
	std::array<hash_t, k_max_texture_slots> texture_slots;
	uint32_t texture_count;
};

struct Material
{
	void load(const fs::path& filepath, const MaterialLayout& layout);
	void release();

	std::array<TextureHandle, k_max_texture_slots> textures;
	size_t texture_count = 0;
};


} // namespace erwin