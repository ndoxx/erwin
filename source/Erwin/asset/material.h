#pragma once

#include <array>
#include "filesystem/filesystem.h"
#include "filesystem/tom_file.h"
#include "render/handles.h"

namespace erwin
{

struct Material
{
	void load(const fs::path& filepath);
	void release();

	std::array<TextureHandle, 8> textures;
	size_t texture_count = 0;
};


} // namespace erwin