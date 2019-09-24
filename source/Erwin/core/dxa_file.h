#pragma once

#include <vector>
#include <functional>
#include <filesystem>

namespace fs = std::filesystem;

namespace erwin
{

// DXA file format
//#pragma pack(push,1)
struct DXAHeader
{
    uint32_t magic;
    uint16_t version_major;
    uint16_t version_minor;
    uint16_t texture_width;
    uint16_t texture_height;
    uint64_t texture_blob_size;
    uint64_t remapping_blob_size;
};
//#pragma pack(pop)
#define DXA_HEADER_SIZE 128
typedef union
{
    struct DXAHeader h;
    uint8_t padding[DXA_HEADER_SIZE];
} DXAHeaderWrapper;

#define DXA_MAGIC 0x41584457 // ASCII(WDXA)
#define DXA_VERSION_MAJOR 1
#define DXA_VERSION_MINOR 0

struct DXAAtlasRemapElement
{
    char     name[32];
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
};

struct DXADescriptor
{
    fs::path filepath;
    void* texture_blob;
    void* remapping_blob;
    uint16_t texture_width;
    uint16_t texture_height;
    uint32_t texture_blob_size;
    uint32_t remapping_blob_size;

    void release();
};

extern void read_dxa(DXADescriptor& desc);
extern void write_dxa(const DXADescriptor& desc);

extern void traverse_remapping(const DXADescriptor& desc, std::function<void(const DXAAtlasRemapElement&)> visit);

} // namespace erwin