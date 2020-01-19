#pragma once

#include <filesystem>

#include "common.h"

namespace fs = std::filesystem;

namespace fudge
{
namespace atlas
{

enum class FileType: uint8_t
{
	PNG,
	CAT
};

struct AtlasExportOptions
{
	BlobCompression blob_compression       = BlobCompression::None;
	TextureCompression texture_compression = TextureCompression::None;
	FileType file_type                     = FileType::PNG;
};

// Initialize font library
extern void init_fonts();
// Fonts library cleanup
extern void release_fonts();
// Create an atlas texture plus a remapping file. The atlas contains each sub-texture found in input directory.
extern void make_atlas(const fs::path& input_dir, const fs::path& output_dir, const AtlasExportOptions& options);
// Read a font file (.ttf) and export an atlas plus a remapping file. The atlas contains each character existing in the font file.
// raster_size (px): allows to scale the characters.
extern void make_font_atlas(const fs::path& input_font, const fs::path& output_dir, const AtlasExportOptions& options, uint32_t raster_size=32);

} // namespace atlas
} // namespace fudge