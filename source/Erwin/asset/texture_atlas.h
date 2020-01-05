#pragma once

#include "EASTL/hash_map.h"

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
	inline uint32_t get_width() const { return width; }
	inline uint32_t get_height() const { return height; }

	TextureHandle texture;
	uint32_t width;
	uint32_t height;

private:
	eastl::hash_map<hash_t, glm::vec4> remapping;
};

struct FontAtlas
{
	struct RemappingElement
	{
		glm::vec4 uvs;
		glm::ivec2 size_px;
		glm::ivec2 bearing;
		int64_t advance;
	};

	// Load texture data from file and prepare descriptor for later renderer submission
	void load(const fs::path& filepath);
	// Release resources
	void release();
	// Return the remapping element corresponding to a given character index
	inline const RemappingElement& get_remapping(uint64_t index) const { return remapping.at(index); }
	inline uint32_t get_width() const { return width; }
	inline uint32_t get_height() const { return height; }

	TextureHandle texture;
	uint32_t width;
	uint32_t height;

private:
	eastl::hash_map<uint64_t, RemappingElement> remapping;
};

} // namespace erwin
