#include <iostream>
#include <fstream>
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

#include "ft2build.h"
#include FT_FREETYPE_H

using namespace rectpack2D;

namespace fs = std::filesystem;

static fs::path self_path_;
static fs::path root_path_;
static fs::path conf_path_;
static fs::path asset_path_;
static fs::path fonts_path_;

static FT_Library ft_;

// For image texture atlas
struct ImageData
{
    stbi_uc* data;
    int width;
    int height;
    int channels;
    std::string name;
};

// For font texture atlas
struct Character
{
    unsigned long index;
    unsigned char* data;
    long advance;   // Offset to advance to next glyph
    unsigned int size_w; // Size of glyph
    unsigned int size_h;
    int bearing_x; // Offset from baseline to left/top of glyph
    int bearing_y;
};

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
        std::cout << "Can't load 'atlas.ini'" << std::endl;
        return false;
    }

    asset_path_ = root_path_ / reader.Get("paths", "upack", "UNKNOWN");
    if(!fs::exists(asset_path_))
    {
        std::cout << "Path " << asset_path_ << " does not exist." << std::endl;
        return false;
    }

    fonts_path_ = root_path_ / reader.Get("paths", "fonts", "UNKNOWN");
    if(!fs::exists(fonts_path_))
    {
        std::cout << "Path " << fonts_path_ << " does not exist." << std::endl;
        return false;
    }

    return true;
}

static rect_wh pack(std::vector<rect_xywh>& rectangles)
{
    // * Configure Rectpack2D
    constexpr bool allow_flip = false;
    const auto max_side = 1000;
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

static bool make_atlas(const fs::path& input_dir, const fs::path& out_atlas, const fs::path& out_remap)
{
    std::vector<rect_xywh> rectangles;
    std::vector<ImageData> images;

    // * Iterate over all files
    for(auto& entry: fs::directory_iterator(input_dir))
    {
        if(entry.is_regular_file())
        {
            //std::cout << "  * [" << images.size() << "] " << entry.path().filename() << ": ";

            // Load image, force 4 channels
            ImageData img;
            img.name = entry.path().stem().string();
            img.data = stbi_load(entry.path().string().c_str(), &img.width, &img.height, &img.channels, 4);
            if(!img.data)
            {
                std::cout << "Error while loading image." << std::endl;
                return false;
            }
            //std::cout << img.width << "x" << img.height << " - " << img.channels << " channels" << std::endl;

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

    for(int ii=0; ii<rectangles.size(); ++ii)
    {
        const rect_xywh& r = rectangles[ii];
        const ImageData& img = images[ii];
        std::cout << "  * [" << ii << "] " << img.name << " " << r.x << " " << r.y << " " << r.w << " " << r.h << std::endl;
        
        // Set remapping file
        ofs << img.name << " " << r.x << " " << out_h-r.y << " " << r.w << " " << r.h << std::endl;

        // Set atlas image data
        for(int xx=0; xx<r.w; ++xx)
        {
            int out_x = r.x + xx;
            for(int yy=0; yy<r.h; ++yy)
            {
                int out_y = r.y + yy;
                output[4 * (out_y * out_w + out_x) + 0] = img.data[4 * (yy * r.w + xx) + 0];
                output[4 * (out_y * out_w + out_x) + 1] = img.data[4 * (yy * r.w + xx) + 1];
                output[4 * (out_y * out_w + out_x) + 2] = img.data[4 * (yy * r.w + xx) + 2];
                output[4 * (out_y * out_w + out_x) + 3] = img.data[4 * (yy * r.w + xx) + 3];
            }
        }
    }
    ofs.close();

    // Export
    std::cout << "-> export: " << fs::relative(out_atlas, root_path_) << std::endl;
    stbi_write_png(out_atlas.string().c_str(), out_w, out_h, 4, output, out_w * 4);

    // Cleanup
    delete[] output;
    for(auto&& img: images)
        stbi_image_free(img.data);

    return true;
}

static bool make_font_atlas(const fs::path& input_font, const fs::path& out_atlas, const fs::path& out_remap)
{
    std::vector<rect_xywh> rectangles;

    // Open font file as binary and load into freetype
    std::ifstream ifs(input_font, std::ios::binary);
    std::vector<char> buffer((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    FT_Face face;
    if(FT_New_Memory_Face(ft_, reinterpret_cast<FT_Byte*>(&buffer[0]), buffer.size(), 0, &face))
    {
        std::cout << "Failed to load font: " << input_font << std::endl;
        return false;
    }

    // Set face size
    uint32_t width = 0;
    uint32_t height = 32;
    FT_Set_Pixel_Sizes(face, width, height);

    // Iterate over each character
    FT_UInt index;
    FT_ULong cc = FT_Get_First_Char(face, &index);
    std::vector<Character> characters;
    while(true)
    // for(unsigned char cc=0; cc<128; ++cc)
    {
        // Load character glyph
        if(FT_Load_Char(face, cc, FT_LOAD_RENDER))
        {
            std::cout << "Failed to load Glyph: \'" << std::to_string(cc) << "\'" << std::endl;
            continue;
        }
        if(face->glyph->bitmap.width == 0 && face->glyph->bitmap.rows == 0)
        {
            std::cout << "Glyph: \'" << std::to_string(cc) << "\' has null size." << std::endl;
        }
        Character character =
        {
            cc,
            nullptr,
            face->glyph->advance.x,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            face->glyph->bitmap_left,
            face->glyph->bitmap_top,
        };
        // Copy bitmap buffer to new buffer, because next iteration will modify this data
        character.data = new unsigned char[character.size_w*character.size_h];
        memcpy(character.data, face->glyph->bitmap.buffer, character.size_w*character.size_h);

        characters.push_back(character);
        rectangles.push_back({0,0,(int)character.size_w,(int)character.size_h});

        // std::cout << std::to_string(cc) << " " << character.size_w << "x" << character.size_h << std::endl;

        cc = FT_Get_Next_Char(face, cc, &index);
        if(!index)
            break;
    }

    // * Find best packing for images
    auto result_size = pack(rectangles);
    int out_w = result_size.w;
    int out_h = result_size.h;
    std::cout << "Resultant bin size: " << out_w << "x" << out_h << std::endl;

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

    for(int ii=0; ii<rectangles.size(); ++ii)
    {
        const rect_xywh& r = rectangles[ii];
        const Character& charac = characters[ii];
        
        // Set remapping file
        ofs << charac.index << " " << r.x << " " << out_h-r.y << " " << r.w << " " << r.h << " "
            << (charac.advance>>6) << " " << charac.bearing_x << " " << charac.bearing_y << std::endl;
        // advance is bitshifted by 6 (2^6=64) to get value in pixels

        // Set atlas image data
        for(int xx=0; xx<r.w; ++xx)
        {
            int out_x = r.x + xx;
            for(int yy=0; yy<r.h; ++yy)
            {
                int out_y = r.y + yy;
                char value = charac.data[yy * r.w + xx];
                output[4 * (out_y * out_w + out_x) + 0] = value ? 255 : 0;
                output[4 * (out_y * out_w + out_x) + 1] = value ? 255 : 0;
                output[4 * (out_y * out_w + out_x) + 2] = value ? 255 : 0;
                output[4 * (out_y * out_w + out_x) + 3] = value;
            }
        }
    }

    ofs.close();

    // Export
    std::cout << "-> export: " << fs::relative(out_atlas, root_path_) << std::endl;
    stbi_write_png(out_atlas.string().c_str(), out_w, out_h, 4, output, out_w * 4);

    // Cleanup
    for(auto&& charac: characters)
        delete[] charac.data;

    FT_Done_Face(face);
    return true;
}


int main()
{
    std::cout << "Atlas Packer utilitary launched." << std::endl;

    // * Locate executable path, root directory, config directory and asset directory
    std::cout << "Locating unpacked assets." << std::endl;
    self_path_ = get_selfpath();
    root_path_ = self_path_.parent_path().parent_path();
    conf_path_ = root_path_ / "config";

    std::cout << "-> Self path: " << self_path_ << std::endl;
    std::cout << "-> Root path: " << root_path_ << std::endl;
    std::cout << "-> Conf path: " << conf_path_ << std::endl;

    if(!read_conf(conf_path_ / "atlas.ini"))
    {
        std::cout << "Could not complete configuration step, exiting." << std::endl;
        exit(0);
    }
    std::cout << "-> Unpacked assets: " << fs::relative(asset_path_, root_path_) << std::endl;

    // * For each sub-directory in upack directory, create an atlas containing every image in it,
    //   whose name is the sub-directory name
    std::cout << "Iterating unpacked assets directories." << std::endl;
    for(auto& entry: fs::directory_iterator(asset_path_))
    {
        if(entry.is_directory())
        {
            std::string dir_name = entry.path().stem().string();
            std::cout << "*  " << dir_name << std::endl;

            fs::path out_atlas = asset_path_.parent_path() / (dir_name + ".png");
            fs::path out_remap = asset_path_.parent_path() / (dir_name + ".txt");
            make_atlas(entry.path(), out_atlas, out_remap);
        }
    }

    // * Generate an atlas for each font in fonts directory
    // Init freetype
    if(FT_Init_FreeType(&ft_))
    {
        std::cout << "Could not init FreeType Library." << std::endl;
        exit(0);
    }

    for(auto& entry: fs::directory_iterator(fonts_path_))
    {
        if(entry.is_regular_file() && entry.path().extension().string().compare("ttf"))
        {
            std::cout << "Processing font: " << entry.path().filename() << std::endl;
            std::string font_name = entry.path().stem().string();
            fs::path out_atlas = fonts_path_.parent_path() / (font_name + ".png");
            fs::path out_remap = asset_path_.parent_path() / (font_name + ".txt");
            make_font_atlas(entry.path(), out_atlas, out_remap);
        }
    }

    // Cleanup freetype

    FT_Done_FreeType(ft_);


    return 0;
}