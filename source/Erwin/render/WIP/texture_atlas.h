#pragma once

#include <map>

#include "render/WIP/main_renderer.h" // TMP: for handles
#include "filesystem/filesystem.h"
#include "filesystem/cat_file.h"
#include "glm/glm.hpp"

namespace erwin
{
namespace WIP
{

struct TextureAtlas
{
	// Load texture data from file and prepare descriptor for later renderer submission
	void load(const fs::path& filepath);
	// Return lower left and upper right uv coordinates for the sub-texture at input key
	inline const glm::vec4& get_uv(hash_t key) const { return remapping.at(key); }
	inline uint32_t get_width() const { return descriptor.texture_width; }
	inline uint32_t get_height() const { return descriptor.texture_height; }

	cat::CATDescriptor descriptor;
	TextureHandle handle;

private:
	std::map<hash_t, glm::vec4> remapping;
};

} // namespace WIP
} // namespace erwin