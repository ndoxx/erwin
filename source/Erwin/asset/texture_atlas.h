#pragma once

#include <map>

#include "render/handles.h"
#include "filesystem/filesystem.h"
#include "filesystem/cat_file.h"
#include "glm/glm.hpp"

namespace erwin
{

struct TextureAtlas
{
	// Load texture data from file and prepare descriptor for later renderer submission
	void load(const fs::path& filepath);
	// Release resources
	void release();
	// Return lower left and upper right uv coordinates for the sub-texture at input key
	inline const glm::vec4& get_uv(hash_t key) const { return remapping.at(key); }
	inline uint32_t get_width() const { return descriptor.texture_width; }
	inline uint32_t get_height() const { return descriptor.texture_height; }

	cat::CATDescriptor descriptor; // TODO: find a better allocation scheme for temporary data
	TextureHandle texture;

private:
	std::map<hash_t, glm::vec4> remapping;
};

} // namespace erwin
