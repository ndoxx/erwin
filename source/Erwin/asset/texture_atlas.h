#pragma once

#include <cstdint>
#include <map>
#include "render/handles.h"
#include "glm/glm.hpp"

namespace erwin
{

struct TextureAtlas
{
	// Return lower left and upper right uv coordinates for the sub-texture at input key
	inline const glm::vec4& get_uv(hash_t key) const { return remapping.at(key); }

	TextureHandle texture;
	uint32_t width;
	uint32_t height;
	std::map<hash_t, glm::vec4> remapping;
};

struct FontAtlas
{
	struct RemappingElement
	{
		glm::vec4 uvs;
	    uint16_t w = 0;
	    uint16_t h = 0;
	    int16_t bearing_x = 0;
	    int16_t bearing_y = 0;
	    uint16_t advance = 0;
	};

	// Return the remapping element corresponding to a given character index
	inline const RemappingElement& get_remapping(uint64_t index) const { return remapping.at(index); }

	TextureHandle texture;
	uint32_t width;
	uint32_t height;
	std::map<uint64_t,RemappingElement> remapping;
};

} // namespace erwin