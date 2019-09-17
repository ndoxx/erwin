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

static bool make_atlas(const fs::path& input_dir, const fs::path& output_file)
{
    // Configure Rectpack2D
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

    // Iterate over all files
    for(auto& entry: fs::directory_iterator(input_dir))
    {
        if(entry.is_regular_file())
        {
            std::cout << "  * [" << images.size() << "] " << entry.path().filename() << ": ";

            ImageData img;
            img.data = stbi_load(entry.path().string().c_str(), &img.width, &img.height, &img.channels, 0);
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

    // Find best packing for images
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

    std::cout << "Resultant bin size: " << result_size.w << "x" << result_size.h << std::endl;

    // Pack images in an atlas
    for(int ii=0; ii<rectangles.size(); ++ii)
    {
        const rect_type& r = rectangles[ii];
        std::cout << "  * [" << ii << "] " << r.x << " " << r.y << " " << r.w << " " << r.h << std::endl;
    }

    // Export
    std::cout << "-> export: " << fs::relative(output_file, root_path_) << std::endl;

    // Cleanup
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

            fs::path output_file = asset_path_.parent_path() / (dir_name + ".png");
            make_atlas(entry.path(), output_file);
        }
    }

    return 0;
}