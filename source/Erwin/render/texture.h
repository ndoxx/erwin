#pragma once

#include <unordered_map>

#include "core/core.h"
#include "filesystem/filesystem.h"
#include "render/texture_common.h"

namespace erwin
{

class Texture
{
public:
	virtual ~Texture() = default;

	virtual uint32_t get_width() const = 0;
	virtual uint32_t get_height() const = 0;

	virtual void bind(uint32_t slot = 0) const = 0;
	virtual void unbind() const = 0;
};


struct Texture2DDescriptor
{
	uint32_t width;
	uint32_t height;
	void* data = nullptr;
	ImageFormat image_format = ImageFormat::RGBA8;
	uint8_t filter = MIN_LINEAR | MAG_NEAREST;
	TextureWrap wrap = TextureWrap::REPEAT;
	bool lazy_mipmap = false;
};

class Texture2D: public Texture
{
public:
	virtual ~Texture2D() = default;

	// Create a 2D texture from a file
	static WRef<Texture2D> create(const fs::path& filepath);
	// Create a 2D texture from descriptor
	static WRef<Texture2D> create(const Texture2DDescriptor& descriptor);
    // Generate a list of textures (maps) from a TOM file
    static std::unordered_map<hash_t, WRef<Texture2D>> create_maps(const fs::path& filepath);
};

} // namespace erwin