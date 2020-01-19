#pragma once

enum class TextureCompression: uint16_t
{
	None = 0,
	DXT1,
	DXT5
};

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
    NONE = 0,
    R8,
    RGB8,
    RGBA8,
    RG16F,
    RGB16F,
    RGBA16F,
    RGB32F,
    RGBA32F,
    SRGB8,
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