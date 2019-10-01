#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace fudge
{

enum class Compression: uint8_t
{
    None = 0,
    DXT,
    Deflate
};

// Initialize font library
extern void init_fonts();
// Fonts library cleanup
extern void release_fonts();
// Set lossless compression option
extern void set_compression(Compression compression); 
// Create an atlas texture plus a remapping file. The atlas contains each sub-texture found in input directory.
extern void make_atlas(const fs::path& input_dir, const fs::path& output_dir, Compression compr = Compression::None);
// Read a font file (.ttf) and export an atlas plus a remapping file. The atlas contains each character existing in the font file.
// raster_size (px): allows to scale the characters.
extern void make_font_atlas(const fs::path& input_font, const fs::path& output_dir, Compression compr = Compression::None, uint32_t raster_size=32);

} // namespace fudge