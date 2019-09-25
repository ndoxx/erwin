#pragma once

/*
    Compressed ATlas file format.
        * DXT5 compression
        * Contains remapping data
*/

#include <vector>
#include <functional>
#include <filesystem>

namespace fs = std::filesystem;

namespace erwin
{

enum class TextureCompression: uint16_t
{
	None = 0,
	DXT1,
	DXT5
};

enum class LosslessCompression: uint16_t
{
	None = 0,
	Deflate
};

// CAT file format
//#pragma pack(push,1)
struct CATHeader
{
    uint32_t magic;                 // Magic number to check file format validity
    uint16_t version_major;         // Version major number
    uint16_t version_minor;         // Version minor number
    uint16_t texture_width;         // Width of texture in pixels
    uint16_t texture_height;        // Height of texture in pixels
    uint16_t texture_compression;   // Type of (lossy) texture compression
    uint16_t lossless_compression;  // Type of (lossless) blob compression
    uint64_t texture_blob_size;     // Size of texture blob
    uint64_t blob_inflate_size;     // Size of inflated texture blob (after blob decompression)
    uint64_t remapping_blob_size;   // Size of remapping table
};
//#pragma pack(pop)
#define CAT_HEADER_SIZE 128
typedef union
{
    struct CATHeader h;
    uint8_t padding[CAT_HEADER_SIZE];
} CATHeaderWrapper;

#define CAT_MAGIC 0x54414357 // ASCII(WCAT)
#define CAT_VERSION_MAJOR 1
#define CAT_VERSION_MINOR 1

struct CATAtlasRemapElement
{
    char     name[32];
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
};

struct CATDescriptor
{
    fs::path filepath;
    void* texture_blob;
    void* remapping_blob;
    uint32_t texture_width;
    uint32_t texture_height;
    uint32_t texture_blob_size;
    uint32_t blob_inflate_size;
    uint32_t remapping_blob_size;
	TextureCompression texture_compression;
	LosslessCompression lossless_compression;
    void release();
};

extern void read_cat(CATDescriptor& desc);
extern void write_cat(const CATDescriptor& desc);

extern void traverse_remapping(const CATDescriptor& desc, std::function<void(const CATAtlasRemapElement&)> visit);

} // namespace erwin