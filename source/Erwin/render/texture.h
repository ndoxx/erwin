#pragma once

#include "core/file_system.h"

namespace erwin
{

enum class TextureCompression: uint16_t;

class Texture
{
public:
	virtual ~Texture() = default;

	virtual uint32_t get_width() const = 0;
	virtual uint32_t get_height() const = 0;

	virtual void bind(uint32_t slot = 0) = 0;
	virtual void unbind() = 0;
};

class Texture2D: public Texture
{
public:
	virtual ~Texture2D() = default;

	// Create a 2D texture from a file
	static std::shared_ptr<Texture2D> create(const fs::path& filepath);
	// Create a 2D texture from data
	static std::shared_ptr<Texture2D> create(void* data, uint32_t width, uint32_t height, TextureCompression compression);
};

} // namespace erwin