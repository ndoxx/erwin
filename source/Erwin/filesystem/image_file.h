#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace erwin
{
namespace img
{

struct HDRDescriptor
{
    fs::path filepath;
    uint32_t channels = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    float* data = nullptr;
};

struct PNGDescriptor
{
    fs::path filepath;
    uint32_t channels = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    unsigned char* data = nullptr;
};

// Read an HDR file, put data into descriptor (will allocate memory)
extern void read_hdr(HDRDescriptor& desc);

// Read a PNG file, put data into descriptor (will allocate memory)
extern void read_png(PNGDescriptor& desc);

} // namespace img
} // namespace erwin