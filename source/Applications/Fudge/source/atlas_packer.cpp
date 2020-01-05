#include "atlas_packer.h"
#include "optimal_packing.h"
#include "dxt_compressor.h"

#include "filesystem/cat_file.h"
#include "debug/logger.h"

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "ft2build.h"
#include FT_FREETYPE_H

#include <fstream>

using namespace erwin;
using namespace rectpack2D;

namespace fudge
{
namespace atlas
{

static Compression s_blob_compression = Compression::Deflate;

// Set lossless compression, only affects CAT export
void set_compression(Compression compression)
{
    s_blob_compression = compression;
}

/*
              _______        _                            _   _           
             |__   __|      | |                      /\  | | | |          
                | | _____  _| |_ _   _ _ __ ___     /  \ | |_| | __ _ ___ 
                | |/ _ \ \/ / __| | | | '__/ _ \   / /\ \| __| |/ _` / __|
                | |  __/>  <| |_| |_| | | |  __/  / ____ \ |_| | (_| \__ \
                |_|\___/_/\_\\__|\__,_|_|  \___| /_/    \_\__|_|\__,_|___/
*/

// For image texture atlas
struct ImageData
{
    std::string name;   // Name of image
    stbi_uc* data;      // Image data
    int x;              // Image position in atlas
    int y;
    int width;          // Size of image
    int height;
    int channels;       // Number of color channels
};

// Blit all input images into a single array, and construct remapping data
static uint8_t* blit_atlas(const std::vector<ImageData>& images, uint32_t width, uint32_t height, bool inverse_y, std::vector<cat::CATAtlasRemapElement>& remap)
{
    // Allocate block data array
    uint8_t* uncomp = new uint8_t[4*width*height];
    memset(uncomp, 0, 4*width*height);

    // Allocate remapping data array
    remap.reserve(images.size());

    // Create the uncompressed atlas texture
    for(int ii=0; ii<images.size(); ++ii)
    {
        const ImageData& img = images[ii];

        // Push remapping element
        cat::CATAtlasRemapElement elt;
        unsigned long str_size = std::min(img.name.size(),31ul);
        strncpy(elt.name, img.name.c_str(), str_size);
        elt.name[str_size] = '\0';

        if(img.name.size()>31)
        {
            DLOGW("fudge") << "Truncated name: " << img.name << " -> " << elt.name << std::endl;
        }

        elt.x = img.x;
        elt.y = height-img.y - img.height; // Bottom left corner y
        elt.w = img.width;
        elt.h = img.height;
        remap.push_back(elt);

        // Set atlas image data
        for(int yy=0; yy<img.height; ++yy)
        {
            int out_y = inverse_y ? (height - 1) - (img.y + yy) : img.y + yy;
            for(int xx=0; xx<img.width; ++xx)
            {
                int out_x = img.x + xx;
                uncomp[4 * (out_y * width + out_x) + 0] = img.data[4 * (yy * img.width + xx) + 0]; // R channel
                uncomp[4 * (out_y * width + out_x) + 1] = img.data[4 * (yy * img.width + xx) + 1]; // G channel
                uncomp[4 * (out_y * width + out_x) + 2] = img.data[4 * (yy * img.width + xx) + 2]; // B channel
                uncomp[4 * (out_y * width + out_x) + 3] = img.data[4 * (yy * img.width + xx) + 3]; // A channel
            }
        }
    }

    return uncomp;
}

// Export texture atlas as a PNG image together with a text file containing remapping data
static void export_atlas_png(uint8_t* uncomp, const std::vector<cat::CATAtlasRemapElement>& remap, const fs::path& output_dir, const std::string& out_name, int out_w, int out_h)
{
    fs::path out_atlas = output_dir / (out_name + ".png");
    fs::path out_remap = output_dir / (out_name + ".txt");

    // Write remapping file
    std::ofstream ofs(out_remap);
    // Write comment for column names
    ofs << "# x: left to right, y: bottom to top, coords are for bottom left corner of sub-image" << std::endl;
    ofs << "# name x y w h" << std::endl;

    for(auto&& elt: remap)
        ofs << elt.name << " " << elt.x << " " << elt.y << " " << elt.w << " " << elt.h << std::endl;

    ofs.close();

    // Export
    DLOGI << "export: " << WCC('p') << out_atlas << std::endl;
    DLOGI << "export: " << WCC('p') << out_remap << std::endl;
    stbi_write_png(out_atlas.string().c_str(), out_w, out_h, 4, uncomp, out_w * 4);
}

// Export texture atlas as a binary CAT file containing both a (compressed) texture blob and a remapping table 
static void export_atlas_cat(uint8_t* uncomp, const std::vector<cat::CATAtlasRemapElement>& remap, const fs::path& output_dir, const std::string& out_name, uint32_t out_w, uint32_t out_h)
{
    uint8_t* tex_blob = fudge::compress_dxt_5(uncomp, out_w, out_h);
    uint32_t dxt_size = out_w*out_h;

    // Export
    fs::path out_atlas = output_dir / (out_name + ".cat");
    DLOGI << "export: " << WCC('p') << out_atlas << std::endl;
    cat::write_cat(
    {
        out_atlas,
        tex_blob,
        (void*)remap.data(),
        out_w,
        out_h,
        dxt_size, // Blob size
        (uint32_t)(remap.size()*sizeof(cat::CATAtlasRemapElement)),
        TextureCompression::DXT5,
        (s_blob_compression == Compression::Deflate) ? cat::LosslessCompression::Deflate : cat::LosslessCompression::None,
        cat::RemappingType::TextureAtlas
    });
    // Cleanup
    delete[] tex_blob;
}

// Explore a directory containing multiple images, load them, blit them, build and export a texture atlas
void make_atlas(const fs::path& input_dir, const fs::path& output_dir, Compression compr)
{
    std::vector<rect_xywh> rectangles;
    std::vector<ImageData> images;

    // * Iterate over all files
    for(auto& entry: fs::directory_iterator(input_dir))
    {
        if(entry.is_regular_file())
        {
            // Load image, force 4 channels
            ImageData img;
            img.name = entry.path().stem().string();
            img.data = stbi_load(entry.path().string().c_str(), &img.width, &img.height, &img.channels, 4);
            img.x = 0;
            img.y = 0;
            if(!img.data)
            {
                DLOGE("fudge") << "Error while loading image: " << entry.path().filename() << std::endl;
                continue;
            }

            // Insert image and rectangle at the same time so they have the same index (stupid rectpack2D lib)
            images.push_back(img);
            rectangles.push_back({0,0,img.width,img.height});
        }
    }

    // * Find best packing for images
    auto result_size = fudge::pack(rectangles);
    int out_w = result_size.w;
    int out_h = result_size.h;
    DLOG("fudge",1) << "Resultant bin size: " << WCC('v') << out_w << "x" << out_h << std::endl;

    // Update image positions
    for(int ii=0; ii<rectangles.size(); ++ii)
    {
        images[ii].x = rectangles[ii].x;
        images[ii].y = rectangles[ii].y;
    }

    // * Generate uncompressed atlas data
    // Pad size to multiple of 4 if necessary
    if(compr == Compression::DXT5)
    {
        if(out_w%4)
        {
            out_w += (4-out_w%4);
            DLOG("fudge",1) << "Padded width to: "  << WCC('v') << out_w << std::endl;
        }
        if(out_h%4)
        {
            out_h += (4-out_h%4);
            DLOG("fudge",1) << "Padded height to: " << WCC('v') << out_h << std::endl;
        }
    }

    // * Export
    std::vector<cat::CATAtlasRemapElement> remap;
    std::string dir_name = input_dir.stem().string();
    uint8_t* uncomp = nullptr;
    switch(compr)
    {
        case Compression::None:
        {
            uncomp = blit_atlas(images, out_w, out_h, false, remap);
            export_atlas_png(uncomp, remap, output_dir, dir_name, out_w, out_h);
            break;
        }
        case Compression::DXT5:
        {
            uncomp = blit_atlas(images, out_w, out_h, true, remap);
            export_atlas_cat(uncomp, remap, output_dir, dir_name, out_w, out_h);
            break;
        }
        default: break;
    }

    // Cleanup
    delete[] uncomp;
    for(auto&& img: images)
        stbi_image_free(img.data);
}

/*
              ______          _             _   _           
             |  ____|        | |       /\  | | | |          
             | |__ ___  _ __ | |_     /  \ | |_| | __ _ ___ 
             |  __/ _ \| '_ \| __|   / /\ \| __| |/ _` / __|
             | | | (_) | | | | |_   / ____ \ |_| | (_| \__ \
             |_|  \___/|_| |_|\__| /_/    \_\__|_|\__,_|___/
*/

// For font texture atlas
struct Character
{
    unsigned long index; // Character index
    unsigned char* data; // Bitmap data
    int x;               // Character position in atlas
    int y;
    unsigned int width; // Size of glyph
    unsigned int height;
    long advance;        // Offset to advance to next glyph
    int bearing_x;       // Offset from baseline to left/top of glyph
    int bearing_y;
};

static FT_Library ft_;

void init_fonts()
{
    // Init freetype
    if(FT_Init_FreeType(&ft_))
    {
        DLOGE("fudge") << "Could not init FreeType Library." << std::endl;
        exit(0);
    }
}

void release_fonts()
{
    // Cleanup freetype
    FT_Done_FreeType(ft_);
}

static uint8_t* blit_font_atlas(const std::vector<Character>& characters, uint32_t width, uint32_t height, bool inverse_y, std::vector<cat::CATFontRemapElement>& remap)
{
    // Allocate block data array
    unsigned char* uncomp = new unsigned char[4*width*height];
    memset(uncomp, 0, 4*width*height);

    // Allocate remapping data array
    remap.reserve(characters.size());

    // Create the uncompressed atlas texture
    for(int ii=0; ii<characters.size(); ++ii)
    {
        const Character& charac = characters[ii];
        
        // Push remapping element
        cat::CATFontRemapElement elt =
        {
            charac.index,
            uint16_t(charac.x),
            uint16_t(height-charac.y - charac.height),
            uint16_t(charac.width),
            uint16_t(charac.height),
            uint16_t(charac.bearing_x),
            uint16_t(charac.bearing_y),
            (charac.advance>>6) // advance is bitshifted by 6 (2^6=64) to get value in pixels
        };
        remap.push_back(elt);

        // Blit atlas image data
        for(int yy=0; yy<charac.height; ++yy)
        {
            int out_y = inverse_y ? (height - 1) - (charac.y + yy) : charac.y + yy;
            for(int xx=0; xx<charac.width; ++xx)
            {
                int out_x = charac.x + xx;
                char value = charac.data[yy * charac.width + xx];
                uncomp[4 * (out_y * width + out_x) + 0] = value ? 255 : 0; // R channel
                uncomp[4 * (out_y * width + out_x) + 1] = value ? 255 : 0; // G channel
                uncomp[4 * (out_y * width + out_x) + 2] = value ? 255 : 0; // B channel
                uncomp[4 * (out_y * width + out_x) + 3] = value;           // A channel
            }
        }
    }

    return uncomp;
}

static void export_font_atlas_png(uint8_t* uncomp, const std::vector<cat::CATFontRemapElement>& remap, const fs::path& output_dir, const std::string& out_name, int out_w, int out_h)
{
    fs::path out_atlas = output_dir / (out_name + ".png");
    fs::path out_remap = output_dir / (out_name + ".txt");

    // Write remapping file
    std::ofstream ofs(out_remap);
    // Write comment for column names
    ofs << "# x: left to right, y: bottom to top, coords are for bottom left corner of sub-image" << std::endl;
    ofs << "# index x y w h bearing_x bearing_y advance" << std::endl;

    for(auto&& elt: remap)
        ofs << elt.index << " " << elt.x << " " << elt.y << " " << elt.w << " " << elt.h << " "
            << elt.bearing_x << " " << elt.bearing_y << " " << elt.advance << std::endl;

    ofs.close();

    // Export
    DLOGI << "export: " << WCC('p') << out_atlas << std::endl;
    DLOGI << "export: " << WCC('p') << out_remap << std::endl;
    stbi_write_png(out_atlas.string().c_str(), out_w, out_h, 4, uncomp, out_w * 4);
}

static void export_font_atlas_cat(uint8_t* uncomp, const std::vector<cat::CATFontRemapElement>& remap, const fs::path& output_dir, const std::string& out_name, uint32_t out_w, uint32_t out_h)
{
    uint8_t* tex_blob = fudge::compress_dxt_5(uncomp, out_w, out_h);
    uint32_t dxt_size = out_w*out_h;

    // Export
    fs::path out_atlas = output_dir / (out_name + ".cat");
    DLOGI << "export: " << WCC('p') << out_atlas << std::endl;
    cat::write_cat(
    {
        out_atlas,
        tex_blob,
        (void*)remap.data(),
        out_w,
        out_h,
        dxt_size, // Blob size
        (uint32_t)(remap.size()*sizeof(cat::CATFontRemapElement)),
        TextureCompression::DXT5,
        (s_blob_compression == Compression::Deflate) ? cat::LosslessCompression::Deflate : cat::LosslessCompression::None,
        cat::RemappingType::FontAtlas
    });

    // Cleanup
    delete[] tex_blob;
}

void make_font_atlas(const fs::path& input_font, const fs::path& output_dir, Compression compr, uint32_t raster_size)
{
    // * Create a new face using Freetype, and load each character
    // Open font file as binary and load into freetype
    std::ifstream ifs(input_font, std::ios::binary);
    std::vector<char> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    FT_Face face;
    if(FT_New_Memory_Face(ft_, reinterpret_cast<FT_Byte*>(&buffer[0]), buffer.size(), 0, &face))
    {
        DLOGE("fudge") << "Failed to load font: " << input_font << std::endl;
        return;
    }

    // Set face size
    uint32_t width = 0;
    uint32_t height = raster_size;
    FT_Set_Pixel_Sizes(face, width, height);

    // Iterate over each character
    FT_UInt index;
    FT_ULong cc = FT_Get_First_Char(face, &index);
    std::vector<Character> characters;
    std::vector<rect_xywh> rectangles;
    while(true)
    {
        // Load character glyph
        if(FT_Load_Char(face, cc, FT_LOAD_RENDER))
        {
            DLOGE("fudge") << "Failed to load Glyph: \'" << std::to_string(cc) << "\'" << std::endl;
            cc = FT_Get_Next_Char(face, cc, &index);
            if(!index)
                break;
            continue;
        }
        // Null size characters (like space and DEL) have no pixel data, they will need special treatment in the engine,
        // but we don't save them in the atlas.
        if(face->glyph->bitmap.width == 0 && face->glyph->bitmap.rows == 0)
        {
            DLOGW("fudge") << "Glyph: \'" << std::to_string(cc) << "\' has null size." << std::endl;
            cc = FT_Get_Next_Char(face, cc, &index);
            if(!index)
                break;
            continue;
        }

        Character character =
        {
            cc,
            nullptr,
            0,
            0,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            face->glyph->advance.x,
            face->glyph->bitmap_left,
            face->glyph->bitmap_top,
        };
        // Copy bitmap buffer to new buffer, because next iteration will modify this data
        character.data = new unsigned char[character.width*character.height];
        memcpy(character.data, face->glyph->bitmap.buffer, character.width*character.height);

        characters.push_back(character);
        rectangles.push_back({0,0,(int)character.width,(int)character.height});

        // Get next character, if index is null it means that we don't have a next character
        cc = FT_Get_Next_Char(face, cc, &index);
        if(!index)
            break;
    }

    // * Find best packing for characters
    auto result_size = fudge::pack(rectangles,1000);
    int out_w = result_size.w;
    int out_h = result_size.h;
    DLOG("fudge",1) << "Resultant bin size: " << WCC('v') << out_w << "x" << out_h << std::endl;

    // Update character positions
    for(int ii=0; ii<rectangles.size(); ++ii)
    {
        characters[ii].x = rectangles[ii].x;
        characters[ii].y = rectangles[ii].y;
    }

    // * Generate uncompressed atlas data
    // Pad size to multiple of 4 if necessary
    if(compr == Compression::DXT5)
    {
        if(out_w%4)
        {
            out_w += (4-out_w%4);
            DLOG("fudge",1) << "Padded width to: "  << WCC('v') << out_w << std::endl;
        }
        if(out_h%4)
        {
            out_h += (4-out_h%4);
            DLOG("fudge",1) << "Padded height to: " << WCC('v') << out_h << std::endl;
        }
    }

    // * Export
    std::vector<cat::CATFontRemapElement> remap;
    std::string font_name = input_font.stem().string();
    // export_font_atlas_png(characters, output_dir, font_name, out_w, out_h);
    uint8_t* uncomp = nullptr;
    switch(compr)
    {
        case Compression::None:
        {
            uncomp = blit_font_atlas(characters, out_w, out_h, false, remap);
            export_font_atlas_png(uncomp, remap, output_dir, font_name, out_w, out_h);
            break;
        }
        case Compression::DXT5:
        {
            uncomp = blit_font_atlas(characters, out_w, out_h, true, remap);
            export_font_atlas_cat(uncomp, remap, output_dir, font_name, out_w, out_h);
            break;
        }
        default: break;
    }

    // Cleanup
    delete[] uncomp;
    for(auto&& charac: characters)
        delete[] charac.data;

    FT_Done_Face(face);
}

} // namespace atlas
} // namespace fudge