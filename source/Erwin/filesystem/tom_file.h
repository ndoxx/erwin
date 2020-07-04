#pragma once

/*
	Texture & Operation Maps file
*/

#include <vector>
#include <string>

#include "core/core.h"
#include "filesystem/file_path.h"
#include "render/texture_common.h"

namespace erwin
{
namespace tom
{

enum class LosslessCompression: uint8_t
{
	None = 0,
	Deflate
};

enum class MaterialType: uint8_t
{
	NONE = 0,
	PBR
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
    FilePath filepath;
    uint16_t width;
    uint16_t height;
    LosslessCompression compression;
    TextureWrap address_UV;
    std::vector<TextureMapDescriptor> texture_maps = {};

    uint8_t* material_data = nullptr;
    uint32_t material_data_size = 0;
    MaterialType material_type = MaterialType::NONE;

    void release();
};

// Read a TOM file, put data into descriptor (will allocate memory)
extern void read_tom(TOMDescriptor& desc);
// Write a TOM file using data contained in descriptor
extern void write_tom(TOMDescriptor& desc);

} // namespace tom
} // namespace erwin