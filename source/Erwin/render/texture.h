#pragma once

#include "core/core.h"
#include "core/file_system.h"

namespace erwin
{

enum TextureFilter: uint8_t
{
    MAG_NEAREST = 0,
    MAG_LINEAR  = 1,

    MIN_NEAREST = 2,
    MIN_LINEAR  = 4,
    MIN_NEAREST_MIPMAP_NEAREST = 8,
    MIN_LINEAR_MIPMAP_NEAREST  = 16,
    MIN_NEAREST_MIPMAP_LINEAR  = 32,
    MIN_LINEAR_MIPMAP_LINEAR   = 64
};

enum class TextureWrap: uint8_t
{
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE
};

enum class ImageFormat: uint8_t
{
    R8,
    RGB8,
    RGBA8,
    RG16F,
    RGB16F,
    RGBA16F,
    RGB32F,
    RGBA32F,
    SRGB_ALPHA,
    RG16_SNORM,
    RGB16_SNORM,
    RGBA16_SNORM,
    COMPRESSED_RGB_S3TC_DXT1,
    COMPRESSED_RGBA_S3TC_DXT1,
    COMPRESSED_RGBA_S3TC_DXT3,
    COMPRESSED_RGBA_S3TC_DXT5,
    COMPRESSED_SRGB_S3TC_DXT1,
    COMPRESSED_SRGB_ALPHA_S3TC_DXT1,
    COMPRESSED_SRGB_ALPHA_S3TC_DXT3,
    COMPRESSED_SRGB_ALPHA_S3TC_DXT5,
    DEPTH_COMPONENT16,
    DEPTH_COMPONENT24,
    DEPTH_COMPONENT32F,
    DEPTH24_STENCIL8,
    DEPTH32F_STENCIL8,
};

enum class TextureCompression: uint16_t;

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
};

} // namespace erwin