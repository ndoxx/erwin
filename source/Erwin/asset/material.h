#pragma once

#include <array>
#include "filesystem/filesystem.h"
#include "render/handles.h"
#include "asset/handles.h"
#include "render/renderer_config.h"

namespace erwin
{

// Enforces a strict texture slot order inside a material's texture array
// TODO: will also be used to check shader compatibility
struct MaterialLayout
{
	std::array<hash_t, k_max_texture_slots> texture_slots;
	uint32_t texture_count;
};

// Set of texture maps used by one or multiple materials
struct TextureGroup
{
	void load(const fs::path& filepath, const MaterialLayout& layout);
	void release();

	std::array<TextureHandle, k_max_texture_slots> textures;
	uint32_t texture_count = 0;
};

// Associates a shader with all uniform and sampler data needed for it to perform
struct Material
{
	ShaderHandle shader;
	TextureGroupHandle texture_group;
	UniformBufferHandle ubo;
	void* data;
	uint32_t data_size;
};

} // namespace erwin