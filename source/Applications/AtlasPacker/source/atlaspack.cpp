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

using namespace rectpack2D;

namespace fs = std::filesystem;

static fs::path self_path_;
static fs::path root_path_;
static fs::path conf_path_;
static fs::path asset_path_;

struct ImageData
{
    stbi_uc* data;
    int width;
    int height;
    int channels;
    std::string name;
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

    return true;
}

static bool make_atlas(const fs::path& input_dir, const fs::path& out_atlas, const fs::path& out_remap)
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

    std::vector<rect_type> rectangles;
    std::vector<ImageData> images;

    // * Iterate over all files
    for(auto& entry: fs::directory_iterator(input_dir))
    {
        if(entry.is_regular_file())
        {
            std::cout << "  * [" << images.size() << "] " << entry.path().filename() << ": ";

            // Load image, force 4 channels
            ImageData img;
            img.name = entry.path().stem().string();
            img.data = stbi_load(entry.path().string().c_str(), &img.width, &img.height, &img.channels, 4);
            if(!img.data)
            {
                std::cout << std::endl << "Error while loading image." << std::endl;
                return false;
            }
            std::cout << img.width << "x" << img.height << " - " << img.channels << " channels" << std::endl;

            // Insert image and rectangle at the same time so they have the same index (stupid rectpack2D lib)
            images.push_back(img);
            rectangles.push_back({0,0,img.width,img.height});
        }
    }

    // * Find best packing for images
    const auto result_size = find_best_packing<spaces_type>
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
        const rect_type& r = rectangles[ii];
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

    return 0;
}