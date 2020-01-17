#pragma once

/*
	Texture & Operation Maps file
*/

#include <vector>
#include <string>
#include <filesystem>

#include "core/core.h"
#include "render/texture_common.h"

namespace fs = std::filesystem;

namespace erwin
{
namespace tom
{

enum class LosslessCompression: uint8_t
{
	None = 0,
	Deflate
};

struct TextureMapDescriptor
{
	TextureFilter filter;
	uint8_t channels;
	bool srgb;
	TextureCompression compression;
	uint32_t size;
	uint8_t* data;
	hash_t name;

    void release();
};

struct TOMDescriptor
{
    fs::path filepath;
    uint16_t width;
    uint16_t height;
    LosslessCompression compression;
    TextureWrap address_UV;
    std::vector<TextureMapDescriptor> texture_maps;

    void release();
};

// Read a TOM file, put data into descriptor (will allocate memory)
extern void read_tom(TOMDescriptor& desc);
// Write a TOM file using data contained in descriptor
extern void write_tom(const TOMDescriptor& desc);

} // namespace tom
} // namespace erwin