#pragma once

#include <array>
#include "render/handles.h"
#include "render/renderer_config.h"

namespace erwin
{

// Enforces a strict texture slot order inside a material's texture array
// TODO: will also be used to check shader compatibility
struct TextureLayout
{
	std::array<hash_t, k_max_texture_slots> texture_slots;
	uint32_t texture_count = 0;
};

// Set of texture maps used by one or multiple materials
struct TextureGroup
{
	std::array<TextureHandle, k_max_texture_slots> textures;
	uint32_t texture_count = 0;

	inline TextureHandle& operator[](uint32_t index)
	{
		K_ASSERT(index<k_max_texture_slots, "TextureGroup index out of bounds.");
		return textures[index];
	}
	inline const TextureHandle& operator[](uint32_t index) const
	{
		K_ASSERT(index<k_max_texture_slots, "TextureGroup index out of bounds.");
		return textures[index];
	}
};

// Associates a shader with all uniform and sampler data needed for it to perform
struct Material
{
	hash_t archetype;
	TextureGroup texture_group;
	hash_t resource_id;
};

} // namespace erwin