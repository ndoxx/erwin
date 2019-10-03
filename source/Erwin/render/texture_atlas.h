#pragma once

#include <unordered_map>

#include "render/texture.h"
#include "filesystem/filesystem.h"
#include "glm/glm.hpp"

namespace erwin
{

class TextureAtlas
{
public:
	TextureAtlas();
	~TextureAtlas();

	void load(const fs::path& filepath);

	// Return lower left and upper right uv coordinates for the sub-texture at input key
	inline const glm::vec4& get_uv(hash_t key) const { return remapping_.at(key); }

	// Get a random sub-texture uvs, for DEBUG purposes
	const glm::vec4& get_random_uv() const;

	inline uint32_t get_width() const  { return texture_->get_width(); }
	inline uint32_t get_height() const { return texture_->get_height(); }

	inline void bind(uint32_t slot = 0) { texture_->bind(slot); }
	inline void unbind()                { texture_->unbind(); }

	inline WRef<Texture2D> get_texture() { return texture_; }

private:
	WRef<Texture2D> texture_;
	std::unordered_map<hash_t, glm::vec4> remapping_;
};


} // namespace erwin