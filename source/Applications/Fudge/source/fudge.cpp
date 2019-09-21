#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>

#ifdef __linux__
    #include <unistd.h>
    #include <climits>
#elif _WIN32

#endif

#include "inih/cpp/INIReader.h"
#include "rectpack2D/src/finders_interface.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "stb/stb_dxt.h"

#include "ft2build.h"
#include FT_FREETYPE_H

using namespace rectpack2D;

namespace fs = std::filesystem;

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

enum class Compression: uint8_t
{
    None = 0,
    DXT5
};

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

struct DXAFontAtlasRemapElement
{
    uint64_t index;
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    uint32_t advance;
    uint16_t bearing_x;
    uint16_t bearing_y;
};

struct DXADescriptor
{
    fs::path output;
    void* tex_blob;
    void* remap_blob;
    uint32_t tex_blob_size;
    uint32_t remap_blob_size;
    uint16_t texture_width;
    uint16_t texture_height;
};

// Write a DXA file given a texture binary blob and a remapping binary blob
static void write_dxa(const DXADescriptor& desc)
{
    DXAHeaderWrapper header;
    header.h.magic               = DXA_MAGIC;
    header.h.version_major       = DXA_VERSION_MAJOR;
    header.h.version_minor       = DXA_VERSION_MINOR;
    header.h.texture_width       = desc.texture_width;
    header.h.texture_height      = desc.texture_height;
    header.h.texture_blob_size   = desc.tex_blob_size;
    header.h.remapping_blob_size = desc.remap_blob_size;

    std::ofstream ofs(desc.output, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));
    ofs.write(reinterpret_cast<const char*>(desc.tex_blob), desc.tex_blob_size);
    ofs.write(reinterpret_cast<const char*>(desc.remap_blob), desc.remap_blob_size);
    ofs.close();
}


static fs::path s_self_path;  // Path to executable
static fs::path s_root_path;  // Path to root directory of Erwin engine
static fs::path s_conf_path;  // Path to configuration directory
static fs::path s_asset_path; // Path to parent directory of unpacked image files
static fs::path s_fonts_path; // Path to fonts folder

static Compression s_tex_compression;
static Compression s_fnt_compression;

static FT_Library ft_;


// Get path to executable
static fs::path get_selfpath()
{
#ifdef __linux__
    char buff[PATH_MAX];
    std::size_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
    if (len != -1)
    {
        buff[len] = '\0';
        return fs::path(buff);
    }
    else
    {
        std::cerr << "Cannot read self path using readlink." << std::endl;
        return fs::path();
    }
#elif _WIN32

    std::cerr << "get_selfpath() not yet implemented." << std::endl;
    return fs::path();

#endif
}

// Read configuration file (.ini) and setup paths
static bool read_conf(const fs::path& path)
{
    std::cout << "Reading atlas tool config file." << std::endl;

    if(!fs::exists(path))
    {
        std::cout << "File " << path << " does not exist." << std::endl;
        return false;
    }

    INIReader reader(path.string().c_str());
    if(reader.ParseError() < 0)
    {
        std::cout << "Can't load 'fudge.ini'" << std::endl;
        return false;
    }

    s_asset_path = s_root_path / reader.Get("paths", "upack", "UNKNOWN");
    if(!fs::exists(s_asset_path))
    {
        std::cout << "Path " << s_asset_path << " does not exist." << std::endl;
        return false;
    }

    s_fonts_path = s_root_path / reader.Get("paths", "fonts", "UNKNOWN");
    if(!fs::exists(s_fonts_path))
    {
        std::cout << "Path " << s_fonts_path << " does not exist." << std::endl;
        return false;
    }

    std::string comp_str = reader.Get("output", "texture_compression", "UNKNOWN");
    if(!comp_str.compare("none"))
        s_tex_compression = Compression::None;
    else if(!comp_str.compare("DXT5"))
        s_tex_compression = Compression::DXT5;
    else
    {
        std::cout << "Unrecognized compression specifier for textures: " << comp_str << std::endl;
        return false;
    }

    comp_str = reader.Get("output", "font_compression", "UNKNOWN");
    if(!comp_str.compare("none"))
        s_fnt_compression = Compression::None;
    else
    {
        std::cout << "Unrecognized compression specifier for fonts: " << comp_str << std::endl;
        return false;
    }

    return true;
}

// Find the optimal packing for a list of rectangles and return the resultant bin size
// Rectangles coordinates (initially set to 0) are modified directly
// max_side: maximal size of a side for the resultant bin
static rect_wh pack(std::vector<rect_xywh>& rectangles, uint32_t max_side=1000)
{
    // * Configure Rectpack2D
    constexpr bool allow_flip = false;
    const auto discard_step = 1;
    const auto runtime_flipping_mode = flipping_option::ENABLED;
    using spaces_type = rectpack2D::empty_spaces<allow_flip, default_empty_spaces>;
    using rect_type = output_rect_t<spaces_type>;

    auto report_successful = [](rect_type&)
    {
        return callback_result::CONTINUE_PACKING;
    };
    auto report_unsuccessful = [](rect_type&)
    {
        return callback_result::ABORT_PACKING;
    };

    return find_best_packing<spaces_type>
    (
        rectangles,
        make_finder_input
        (
            max_side,
            discard_step,
            report_successful,
            report_unsuccessful,
            runtime_flipping_mode
        )
    );
}

// Export a texture atlas to PNG format with a text remapping file
static void export_atlas_png(const std::vector<ImageData>& images, const std::string& out_name, int out_w, int out_h)
{
    fs::path out_atlas = s_asset_path.parent_path() / (out_name + ".png");
    fs::path out_remap = s_asset_path.parent_path() / (out_name + ".txt");

    // * Pack images in an atlas
    // Allocate output data array
    unsigned char* output = new unsigned char[4*out_w*out_h];
    for(int ii=0; ii<4*out_w*out_h; ++ii)
        output[ii] = 0;

    // Open remapping file
    std::ofstream ofs(out_remap);
    // Write comment for column names
    ofs << "# x: left to right, y: bottom to top, coords are for top left corner of sub-image" << std::endl;
    ofs << "# name x y w h" << std::endl;

    // Create the atlas texture
    for(int ii=0; ii<images.size(); ++ii)
    {
        const ImageData& img = images[ii];
        
        // Set remapping file
        ofs << img.name << " " << img.x << " " << out_h-img.y << " " << img.width << " " << img.height << std::endl;

        // Set atlas image data
        for(int xx=0; xx<img.width; ++xx)
        {
            int out_x = img.x + xx;
            for(int yy=0; yy<img.height; ++yy)
            {
                int out_y = img.y + yy;
                output[4 * (out_y * out_w + out_x) + 0] = img.data[4 * (yy * img.width + xx) + 0]; // R channel
                output[4 * (out_y * out_w + out_x) + 1] = img.data[4 * (yy * img.width + xx) + 1]; // G channel
                output[4 * (out_y * out_w + out_x) + 2] = img.data[4 * (yy * img.width + xx) + 2]; // B channel
                output[4 * (out_y * out_w + out_x) + 3] = img.data[4 * (yy * img.width + xx) + 3]; // A channel
            }
        }
    }
    ofs.close();

    // Export
    std::cout << "-> export: " << fs::relative(out_atlas, s_root_path) << std::endl;
    std::cout << "-> export: " << fs::relative(out_remap, s_root_path) << std::endl;
    stbi_write_png(out_atlas.string().c_str(), out_w, out_h, 4, output, out_w * 4);

    // Cleanup
    delete[] output;
}

// Export a texture atlas to compressed DXA format (remapping information inside)
static void export_atlas_dxt(const std::vector<ImageData>& images, const std::string& out_name, int out_w, int out_h)
{
    // * Pack images in an atlas
    // Pad size to multiple of 4
    if(out_w%4)
    {
        out_w += (4-out_w%4);
        std::cout << "Padded width to: " << out_w << std::endl;
    }
    if(out_h%4)
    {
        out_h += (4-out_h%4);
        std::cout << "Padded height to: " << out_h << std::endl;
    }

    // Allocate block data array
    unsigned char* blocks = new unsigned char[4*out_w*out_h];
    for(int ii=0; ii<4*out_w*out_h; ++ii)
        blocks[ii] = 0;

    // Allocate remapping data array
    std::vector<DXAAtlasRemapElement> remap;
    remap.reserve(images.size());

    // Populate 4x4 blocks from images data
    for(int ii=0; ii<images.size(); ++ii)
    {
        const ImageData& img = images[ii];

        // Push remapping element
        DXAAtlasRemapElement elt;
        memcpy(elt.name, img.name.c_str(), std::min(img.name.size(),31ul));
        elt.x = img.x;
        elt.y = out_h-img.y;
        elt.w = img.width;
        elt.h = img.height;
        remap.push_back(elt);
        
        // Set atlas image data
        for(int xx=0; xx<img.width; ++xx)
        {
            int out_x = img.x + xx;
            for(int yy=0; yy<img.height; ++yy)
            {
                int out_y = img.y + yy;
                // Calculate offset inside block container
                int block_index = out_w * (out_y/4) + (out_x/4);
                int offset = 4*(block_index + 4*(out_y%4) + (out_x%4));
                blocks[offset + 0] = img.data[4 * (yy * img.width + xx) + 0]; // R channel
                blocks[offset + 1] = img.data[4 * (yy * img.width + xx) + 1]; // G channel
                blocks[offset + 2] = img.data[4 * (yy * img.width + xx) + 2]; // B channel
                blocks[offset + 3] = img.data[4 * (yy * img.width + xx) + 3]; // A channel
            }
        }
    }

    // Allocate compressed data array
    const int num_blocks = (out_w*out_h)/16;
    static const int block_size = 16*4; // 4 bytes per-pixel, 16 pixel in a 4x4 block
    static const int compr_size = 16;   // 128 bits compressed size per block
    unsigned char* tex_blob = new unsigned char[num_blocks*compr_size];

    // Compress each block
    for(int ii=0; ii<num_blocks; ++ii)
    {
        int src_offset = ii*block_size;
        int dst_offset = ii*compr_size;
        stb_compress_dxt_block(&tex_blob[dst_offset], &blocks[src_offset], 1, STB_DXT_NORMAL);
    }

    // Export
    fs::path out_atlas = s_asset_path.parent_path() / (out_name + ".dxa");
    std::cout << "-> export: " << fs::relative(out_atlas, s_root_path) << std::endl;
    write_dxa(
    {
        out_atlas,
        tex_blob,
        remap.data(),
        (uint32_t)(num_blocks*compr_size),
        (uint32_t)(remap.size()*sizeof(DXAAtlasRemapElement)),
        (uint16_t)out_w,
        (uint16_t)out_h
    });

    // Cleanup
    delete[] blocks;
    delete[] tex_blob;
}

// Export a font atlas to PNG format with a text remapping file
static void export_font_atlas_png(const std::vector<Character>& characters, const std::string& out_name, int out_w, int out_h)
{
    fs::path out_atlas = s_asset_path.parent_path() / (out_name + ".png");
    fs::path out_remap = s_asset_path.parent_path() / (out_name + ".txt");

    // * Pack images in an atlas
    // Allocate output data array
    unsigned char* output = new unsigned char[4*out_w*out_h];
    for(int ii=0; ii<4*out_w*out_h; ++ii)
        output[ii] = 0;

    // Open remapping file
    std::ofstream ofs(out_remap);
    // Write comment for column names
    ofs << "# x: left to right, y: bottom to top, coords are for top left corner of sub-image" << std::endl;
    ofs << "# index x y w h advance bearing_x bearing_y" << std::endl;

    // Create the atlas texture
    for(int ii=0; ii<characters.size(); ++ii)
    {
        const Character& charac = characters[ii];
        
        // Set remapping file
        // advance is bitshifted by 6 (2^6=64) to get value in pixels
        ofs << charac.index << " " << charac.x << " " << out_h-charac.y << " " << charac.width << " " << charac.height << " "
            << (charac.advance>>6) << " " << charac.bearing_x << " " << charac.bearing_y << std::endl;

        // Set atlas image data
        for(int xx=0; xx<charac.width; ++xx)
        {
            int out_x = charac.x + xx;
            for(int yy=0; yy<charac.height; ++yy)
            {
                int out_y = charac.y + yy;
                char value = charac.data[yy * charac.width + xx];
                output[4 * (out_y * out_w + out_x) + 0] = value ? 255 : 0; // R channel
                output[4 * (out_y * out_w + out_x) + 1] = value ? 255 : 0; // G channel
                output[4 * (out_y * out_w + out_x) + 2] = value ? 255 : 0; // B channel
                output[4 * (out_y * out_w + out_x) + 3] = value;           // A channel
            }
        }
    }
    ofs.close();

    // Export
    std::cout << "-> export: " << fs::relative(out_atlas, s_root_path) << std::endl;
    std::cout << "-> export: " << fs::relative(out_remap, s_root_path) << std::endl;
    stbi_write_png(out_atlas.string().c_str(), out_w, out_h, 4, output, out_w * 4);
}

// Create an atlas texture plus a remapping file. The atlas contains each sub-texture found in input directory.
static void make_atlas(const fs::path& input_dir, Compression compr = Compression::None)
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
                std::cout << "Error while loading image: " << entry.path().filename() << std::endl;
                continue;
            }

            // Insert image and rectangle at the same time so they have the same index (stupid rectpack2D lib)
            images.push_back(img);
            rectangles.push_back({0,0,img.width,img.height});
        }
    }

    // * Find best packing for images
    auto result_size = pack(rectangles);
    int out_w = result_size.w;
    int out_h = result_size.h;
    std::cout << "Resultant bin size: " << out_w << "x" << out_h << std::endl;

    // Update image positions
    for(int ii=0; ii<rectangles.size(); ++ii)
    {
        images[ii].x = rectangles[ii].x;
        images[ii].y = rectangles[ii].y;
    }

    // * Export
    std::string dir_name = input_dir.stem().string();
    switch(compr)
    {
        case Compression::None: export_atlas_png(images, dir_name, out_w, out_h); break;
        case Compression::DXT5: export_atlas_dxt(images, dir_name, out_w, out_h); break;
    }

    // Cleanup
    for(auto&& img: images)
        stbi_image_free(img.data);
}

// Read a font file (.ttf) and export an atlas plus a remapping file. The atlas contains each character existing in the font file.
// raster_size (px): allows to scale the characters.
static void make_font_atlas(const fs::path& input_font, Compression compr = Compression::None, uint32_t raster_size=32)
{
    // * Create a new face using Freetype, and load each character
    // Open font file as binary and load into freetype
    std::ifstream ifs(input_font, std::ios::binary);
    std::vector<char> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    FT_Face face;
    if(FT_New_Memory_Face(ft_, reinterpret_cast<FT_Byte*>(&buffer[0]), buffer.size(), 0, &face))
    {
        std::cout << "Failed to load font: " << input_font << std::endl;
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
            std::cout << "Failed to load Glyph: \'" << std::to_string(cc) << "\'" << std::endl;
            cc = FT_Get_Next_Char(face, cc, &index);
            if(!index)
                break;
            continue;
        }
        // Null size characters (like space and DEL) have no pixel data, they will need special treatment in the engine,
        // but we don't save them in the atlas.
        if(face->glyph->bitmap.width == 0 && face->glyph->bitmap.rows == 0)
        {
            std::cout << "Glyph: \'" << std::to_string(cc) << "\' has null size." << std::endl;
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
    auto result_size = pack(rectangles,1000);
    int out_w = result_size.w;
    int out_h = result_size.h;
    std::cout << "Resultant bin size: " << out_w << "x" << out_h << std::endl;

    // Update character positions
    for(int ii=0; ii<rectangles.size(); ++ii)
    {
        characters[ii].x = rectangles[ii].x;
        characters[ii].y = rectangles[ii].y;
    }

    // * Export
    std::string font_name = input_font.stem().string();
    export_font_atlas_png(characters, font_name, out_w, out_h);

    // Cleanup
    for(auto&& charac: characters)
        delete[] charac.data;

    FT_Done_Face(face);
}

static std::string grad(float a)
{
    std::stringstream ss;
    static const float R1 = 255.f; static const float R2 = 174.f;
    static const float G1 = 200.f; static const float G2 = 7.f;
    static const float B1 = 0.f;   static const float B2 = 7.f;

    int R = int((1.f-a)*R1 + a*R2);
    int G = int((1.f-a)*G1 + a*G2);
    int B = int((1.f-a)*B1 + a*B2);

    ss << "\033[1;38;2;" << R << ";" << G << ";" << B << "m";
    return ss.str();
}

static const std::string S_SECTION = "\033[1;38;2;255;255;153m";
static const std::string S_DEF     = "\033[0m";

static void show_logo()
{
    std::cout << grad(0.0f) << " (                              (                                " << std::endl;
    std::cout << grad(0.2f) << " )\\ )        (                  )\\ )               )             " << std::endl;
    std::cout << grad(0.4f) << "(()/(   (    )\\ )  (  (     (  (()/(    )       ( /(    (   (    " << std::endl;
    std::cout << grad(0.5f) << " /(_)) ))\\  (()/(  )\\))(   ))\\  /(_))( /(   (   )\\())  ))\\  )(   " << std::endl;
    std::cout << grad(0.6f) << "(_))_|/((_)  ((_))((_))\\  /((_)(_))  )(_))  )\\ ((_)\\  /((_)(()\\  " << std::endl;
    std::cout << grad(0.7f) << "| |_ (_))(   _| |  (()(_)(_))  | _ \\((_)_  ((_)| |(_)(_))   ((_) " << std::endl;
    std::cout << grad(0.8f) << "| __|| || |/ _` | / _` | / -_) |  _// _` |/ _| | / / / -_) | '_| " << std::endl;
    std::cout << grad(0.9f) << "|_|   \\_,_|\\__,_| \\__, | \\___| |_|  \\__,_|\\__| |_\\_\\ \\___| |_|   " << std::endl;
    std::cout << grad(1.0f) << "                  |___/                                          \033[0m" << std::endl;
    std::cout << "\033[1;48;2;153;0;0m                        Atlas packer tool                        \033[0m" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char const *argv[])
{
    show_logo();

    // * Locate executable path, root directory, config directory, asset and fonts directories
    std::cout << S_SECTION << "Locating unpacked assets." << S_DEF << std::endl;
    s_self_path = get_selfpath();
    s_root_path = s_self_path.parent_path().parent_path();
    s_conf_path = s_root_path / "config";

    std::cout << "-> Self path: " << s_self_path << std::endl;
    std::cout << "-> Root path: " << s_root_path << std::endl;
    std::cout << "-> Conf path: " << s_conf_path << std::endl;

    if(!read_conf(s_conf_path / "fudge.ini"))
    {
        std::cout << "Could not complete configuration step, exiting." << std::endl;
        exit(0);
    }
    std::cout << "-> Unpacked assets: " << fs::relative(s_asset_path, s_root_path) << std::endl;
    std::cout << "-> Fonts:           " << fs::relative(s_fonts_path, s_root_path) << std::endl;

    std::cout << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    // * For each sub-directory in upack directory, create an atlas containing every image in it,
    //   whose name is the sub-directory name
    std::cout << S_SECTION << "Iterating unpacked assets directories." << S_DEF << std::endl;
    for(auto& entry: fs::directory_iterator(s_asset_path))
    {
        if(entry.is_directory())
        {
            std::cout << "*  Processing directory: " << entry.path().stem() << std::endl;

            make_atlas(entry.path(), s_tex_compression);
            std::cout << std::endl;
        }
    }

    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    // * Generate an atlas for each font in fonts directory
    // Init freetype
    if(FT_Init_FreeType(&ft_))
    {
        std::cout << "Could not init FreeType Library." << std::endl;
        exit(0);
    }

    std::cout << S_SECTION << "Iterating fonts." << S_DEF << std::endl;
    for(auto& entry: fs::directory_iterator(s_fonts_path))
    {
        if(entry.is_regular_file() && entry.path().extension().string().compare("ttf"))
        {
            std::cout << "*  Processing font: " << entry.path().filename() << std::endl;
            std::string font_name = entry.path().stem().string();
            make_font_atlas(entry.path(), s_fnt_compression);
            std::cout << std::endl;
        }
    }

    // Cleanup freetype
    FT_Done_FreeType(ft_);


    return 0;
}