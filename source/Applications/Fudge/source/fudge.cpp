#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#ifdef __linux__
    #include <unistd.h>
    #include <climits>
#elif _WIN32

#endif

#include "inih/cpp/INIReader.h"

#include "atlas_packer.h"


static fs::path s_self_path;  // Path to executable
static fs::path s_root_path;  // Path to root directory of Erwin engine
static fs::path s_conf_path;  // Path to configuration directory
static fs::path s_asset_path; // Path to parent directory of unpacked image files
static fs::path s_fonts_path; // Path to fonts folder

static fudge::Compression s_tex_compression;
static fudge::Compression s_fnt_compression;

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
        s_tex_compression = fudge::Compression::None;
    else if(!comp_str.compare("DXT"))
        s_tex_compression = fudge::Compression::DXT;
    else
    {
        std::cout << "Unrecognized compression specifier for textures: " << comp_str << std::endl;
        return false;
    }

    comp_str = reader.Get("output", "font_compression", "UNKNOWN");
    if(!comp_str.compare("none"))
        s_fnt_compression = fudge::Compression::None;
    else
    {
        std::cout << "Unrecognized compression specifier for fonts: " << comp_str << std::endl;
        return false;
    }

    comp_str = reader.Get("output", "blob_compression", "UNKNOWN");
    if(!comp_str.compare("none"))
        fudge::set_compression(fudge::Compression::None);
    else if(!comp_str.compare("deflate"))
        fudge::set_compression(fudge::Compression::Deflate);
    else
    {
        std::cout << "Unrecognized compression specifier for blob: " << comp_str << std::endl;
        return false;
    }

    return true;
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

            make_atlas(entry.path(), s_asset_path.parent_path(), s_tex_compression);
            std::cout << std::endl;
        }
    }

    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    // * Generate an atlas for each font in fonts directory
    fudge::init_fonts();

    std::cout << S_SECTION << "Iterating fonts." << S_DEF << std::endl;
    for(auto& entry: fs::directory_iterator(s_fonts_path))
    {
        if(entry.is_regular_file() && entry.path().extension().string().compare("ttf"))
        {
            std::cout << "*  Processing font: " << entry.path().filename() << std::endl;
            std::string font_name = entry.path().stem().string();
            make_font_atlas(entry.path(), s_asset_path.parent_path(), s_fnt_compression, 32);
            std::cout << std::endl;
        }
    }

    fudge::release_fonts();

    return 0;
}