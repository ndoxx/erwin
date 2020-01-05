#pragma once

/*
    Compressed ATlas file format.
        * DXT5 compression
        * Contains remapping data
*/

#include <vector>
#include <functional>
#include <filesystem>

#include "render/texture_common.h"

namespace fs = std::filesystem;

namespace erwin
{
namespace cat
{

enum class LosslessCompression: uint16_t
{
	None = 0,
	Deflate
};

enum class RemappingType: uint16_t
{
    TextureAtlas = 0,
    FontAtlas
};

struct CATAtlasRemapElement
{
    char     name[32];
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
};

struct CATFontRemapElement
{
    uint64_t index;
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    int16_t bearing_x;
    int16_t bearing_y;
    uint16_t advance;
};

struct CATDescriptor
{
    fs::path filepath;
    void* texture_blob;
    void* remapping_blob;
    uint32_t texture_width;
    uint32_t texture_height;
    uint32_t texture_blob_size;
    uint32_t remapping_blob_size;
	TextureCompression texture_compression;
	LosslessCompression lossless_compression;
    RemappingType remapping_type;
    
    void release();
};

extern void read_cat(CATDescriptor& desc);
extern void write_cat(const CATDescriptor& desc);

extern void traverse_texture_remapping(const CATDescriptor& desc, std::function<void(const CATAtlasRemapElement&)> visit);
extern void traverse_font_remapping(const CATDescriptor& desc, std::function<void(const CATFontRemapElement&)> visit);

} // namespace cat
} // namespace erwin