#pragma once

#include <array>

namespace erwin
{

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

static bool is_floating_point(ImageFormat format)
{
    return(format == ImageFormat::RG16F ||
           format == ImageFormat::RGB16F ||
           format == ImageFormat::RGBA16F ||
           format == ImageFormat::RGB32F ||
           format == ImageFormat::RGBA32F);
}

enum TextureFlags: uint8_t
{
    TF_NONE        = 0,
    TF_LAZY_MIPMAP = (1<<0),
    TF_MUST_FREE   = (1<<1)
};

using TMEnum = uint8_t;

enum TextureMapType: TMEnum
{
    TM_ALBEDO     = 0,
    TM_NORMAL     = 1,
    TM_DEPTH      = 2,
    TM_METAL      = 3,
    TM_AO         = 4,
    TM_ROUGHNESS  = 5,
    TM_EMISSIVITY = 6,

    TM_COUNT
};

enum TextureMapFlag: TMEnum
{
    TMF_NONE       = 0,
    TMF_ALBEDO     = (1<<TM_ALBEDO),
    TMF_NORMAL     = (1<<TM_NORMAL),
    TMF_DEPTH      = (1<<TM_DEPTH),
    TMF_METAL      = (1<<TM_METAL),
    TMF_AO         = (1<<TM_AO),
    TMF_ROUGHNESS  = (1<<TM_ROUGHNESS),
    TMF_EMISSIVITY = (1<<TM_EMISSIVITY)
};

static constexpr TMEnum TMF_PACK_A    = TMF_ALBEDO;
static constexpr TMEnum TMF_PACK_ND   = TMF_NORMAL | TMF_DEPTH;
static constexpr TMEnum TMF_PACK_MARE = TMF_METAL | TMF_AO | TMF_ROUGHNESS | TMF_EMISSIVITY;

struct Texture2DDescriptor
{
    uint32_t width;
    uint32_t height;
    uint32_t mips = 0;
    void* data = nullptr;
    ImageFormat image_format = ImageFormat::RGBA8;
    uint8_t filter = MIN_LINEAR | MAG_NEAREST;
    TextureWrap wrap = TextureWrap::REPEAT;
    uint8_t flags = TextureFlags::TF_NONE;

    inline bool lazy_mipmap() const { return flags & TextureFlags::TF_LAZY_MIPMAP; }
    inline bool must_free() const   { return flags & TextureFlags::TF_MUST_FREE; }
    void release()
    {
        if(must_free())
        {
            if(is_floating_point(image_format))
                delete[] (static_cast<float*>(data));
            else
                delete[] (static_cast<uint8_t*>(data));
            data = nullptr;
        }
    }
};

struct CubemapDescriptor
{
    uint32_t width;
    uint32_t height;
    uint32_t mips;
    std::array<void*,6> face_data = {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};
    ImageFormat image_format = ImageFormat::RGB16F;
    uint8_t filter = MIN_LINEAR | MAG_LINEAR;
    TextureWrap wrap = TextureWrap::CLAMP_TO_EDGE;
    bool lazy_mipmap = false;
};

struct PixelData
{
    uint8_t* data;
    size_t size;
};

} // namespace erwin