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
#include "texture_packer.h"
#include "debug/logger.h"
#include "debug/logger_thread.h"

using namespace erwin;

static fs::path s_self_path;  // Path to executable
static fs::path s_root_path;  // Path to root directory of Erwin engine
static fs::path s_conf_path;  // Path to configuration directory
static fs::path s_atlas_upack_path; // Path to parent directory of unpacked atlas image files
static fs::path s_atlas_fonts_path; // Path to fonts folder
static fs::path s_tmap_upack_path;  // Path to parent directory of unpacked texture maps
static fs::path s_tmap_config_path; // Path to config file specifying the different texture maps

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
        DLOGE("fudge") << "Cannot read self path using readlink." << std::endl;
        return fs::path();
    }
#elif _WIN32

    DLOGE("fudge") << "get_selfpath() not yet implemented." << std::endl;
    return fs::path();

#endif
}

// Read configuration file (.ini) and setup paths
static bool read_conf(const fs::path& path)
{
    DLOGN("fudge") << "Reading atlas tool config file." << std::endl;

    if(!fs::exists(path))
    {
        DLOGE("fudge") << "File " << path << " does not exist." << std::endl;
        return false;
    }

    INIReader reader(path.string().c_str());
    if(reader.ParseError() < 0)
    {
        DLOGE("fudge") << "Can't load 'fudge.ini'" << std::endl;
        return false;
    }

    s_atlas_upack_path = s_root_path / reader.Get("atlas", "upack_path", "UNKNOWN");
    if(!fs::exists(s_atlas_upack_path))
    {
        DLOGE("fudge") << "Path " << s_atlas_upack_path << " does not exist." << std::endl;
        return false;
    }

    s_atlas_fonts_path = s_root_path / reader.Get("atlas", "fonts_path", "UNKNOWN");
    if(!fs::exists(s_atlas_fonts_path))
    {
        DLOGE("fudge") << "Path " << s_atlas_fonts_path << " does not exist." << std::endl;
        return false;
    }

    std::string comp_str = reader.Get("atlas", "texture_compression", "UNKNOWN");
    if(!comp_str.compare("none"))
        s_tex_compression = fudge::Compression::None;
    else if(!comp_str.compare("DXT5"))
        s_tex_compression = fudge::Compression::DXT5;
    else
    {
        DLOGE("fudge") << "Unrecognized compression specifier for textures: " << comp_str << std::endl;
        return false;
    }

    comp_str = reader.Get("atlas", "font_compression", "UNKNOWN");
    if(!comp_str.compare("none"))
        s_fnt_compression = fudge::Compression::None;
    else
    {
        DLOGE("fudge") << "Unrecognized compression specifier for fonts: " << comp_str << std::endl;
        return false;
    }

    comp_str = reader.Get("atlas", "blob_compression", "UNKNOWN");
    if(!comp_str.compare("none"))
        fudge::atlas::set_compression(fudge::Compression::None);
    else if(!comp_str.compare("deflate"))
        fudge::atlas::set_compression(fudge::Compression::Deflate);
    else
    {
        DLOGE("fudge") << "Unrecognized compression specifier for blob: " << comp_str << std::endl;
        return false;
    }

    s_tmap_upack_path = s_root_path / reader.Get("texmap", "upack_path", "UNKNOWN");
    if(!fs::exists(s_tmap_upack_path))
    {
        DLOGE("fudge") << "Path " << s_tmap_upack_path << " does not exist." << std::endl;
        return false;
    }

    s_tmap_config_path = s_root_path / reader.Get("texmap", "config", "UNKNOWN");
    if(!fs::exists(s_tmap_upack_path))
    {
        DLOGE("fudge") << "Path " << s_tmap_config_path << " does not exist." << std::endl;
        return false;
    }

    comp_str = reader.Get("texmap", "blob_compression", "UNKNOWN");
    if(!comp_str.compare("none"))
        fudge::texmap::set_compression(fudge::Compression::None);
    else if(!comp_str.compare("deflate"))
        fudge::texmap::set_compression(fudge::Compression::Deflate);
    else
    {
        DLOGE("fudge") << "Unrecognized compression specifier for blob: " << comp_str << std::endl;
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

static void show_logo()
{
    DLOGR("fudge") << grad(0.0f) << " (                              (                                " << std::endl;
    DLOGR("fudge") << grad(0.2f) << " )\\ )        (                  )\\ )               )             " << std::endl;
    DLOGR("fudge") << grad(0.4f) << "(()/(   (    )\\ )  (  (     (  (()/(    )       ( /(    (   (    " << std::endl;
    DLOGR("fudge") << grad(0.5f) << " /(_)) ))\\  (()/(  )\\))(   ))\\  /(_))( /(   (   )\\())  ))\\  )(   " << std::endl;
    DLOGR("fudge") << grad(0.6f) << "(_))_|/((_)  ((_))((_))\\  /((_)(_))  )(_))  )\\ ((_)\\  /((_)(()\\  " << std::endl;
    DLOGR("fudge") << grad(0.7f) << "| |_ (_))(   _| |  (()(_)(_))  | _ \\((_)_  ((_)| |(_)(_))   ((_) " << std::endl;
    DLOGR("fudge") << grad(0.8f) << "| __|| || |/ _` | / _` | / -_) |  _// _` |/ _| | / / / -_) | '_| " << std::endl;
    DLOGR("fudge") << grad(0.9f) << "|_|   \\_,_|\\__,_| \\__, | \\___| |_|  \\__,_|\\__| |_\\_\\ \\___| |_|   " << std::endl;
    DLOGR("fudge") << grad(1.0f) << "                  |___/                                          \033[0m" << std::endl;
    DLOGR("fudge") << "\033[1;48;2;153;0;0m                        Atlas packer tool                        \033[0m" << std::endl;
    DLOGR("fudge") << std::endl;
}

static void init_logger()
{
    WLOGGER.create_channel("fudge", 3);
    WLOGGER.attach_all("ConsoleSink", std::make_unique<dbg::ConsoleSink>());
    WLOGGER.attach_all("MainFileSink", std::make_unique<dbg::LogFileSink>("fudge.log"));
    WLOGGER.set_single_threaded(true);
}

int main(int argc, char const *argv[])
{
    init_logger();
    show_logo();

    // * Locate executable path, root directory, config directory, asset and fonts directories
    DLOGN("fudge") << "Locating unpacked assets." << std::endl;
    s_self_path = get_selfpath();
    s_root_path = s_self_path.parent_path().parent_path();
    s_conf_path = s_root_path / "config";

    DLOGI << "Self path: " << WCC('p') << s_self_path << std::endl;
    DLOGI << "Root path: " << WCC('p') << s_root_path << std::endl;
    DLOGI << "Conf path: " << WCC('p') << s_conf_path << std::endl;

    if(!read_conf(s_conf_path / "fudge.ini"))
    {
        DLOGE("fudge") << "Could not complete configuration step, exiting." << std::endl;
        exit(0);
    }
    DLOGI << "Atlas unpacked:  " << WCC('p') << fs::relative(s_atlas_upack_path, s_root_path) << std::endl;
    DLOGI << "Atlas fonts:     " << WCC('p') << fs::relative(s_atlas_fonts_path, s_root_path) << std::endl;
    DLOGI << "Texmap unpacked: " << WCC('p') << fs::relative(s_tmap_upack_path, s_root_path) << std::endl;

    DLOGR("fudge") << std::endl;
    DLOGR("fudge") << "--------------------------------------------------------------------------------" << std::endl;
    DLOGR("fudge") << std::endl;

    // * For each sub-directory in upack directory, create an atlas containing every image in it,
    //   whose name is the sub-directory name
    DLOGN("fudge") << "Iterating unpacked atlases directories." << std::endl;
    for(auto& entry: fs::directory_iterator(s_atlas_upack_path))
    {
        if(entry.is_directory())
        {
            DLOG("fudge",1) << "Processing directory: " << WCC('p') << entry.path().stem() << std::endl;

            fudge::atlas::make_atlas(entry.path(), s_atlas_upack_path.parent_path(), s_tex_compression);
            DLOGR("fudge") << std::endl;
        }
    }

    DLOGR("fudge") << "--------------------------------------------------------------------------------" << std::endl;
    DLOGR("fudge") << std::endl;

    // * Generate an atlas for each font in fonts directory
    fudge::atlas::init_fonts();

    DLOGN("fudge") << "Iterating fonts." << std::endl;
    for(auto& entry: fs::directory_iterator(s_atlas_fonts_path))
    {
        if(entry.is_regular_file() && entry.path().extension().string().compare("ttf"))
        {
            DLOG("fudge",1) << "Processing font: " << WCC('n') << entry.path().filename() << std::endl;
            std::string font_name = entry.path().stem().string();
            fudge::atlas::make_font_atlas(entry.path(), s_atlas_upack_path.parent_path(), s_fnt_compression, 32);
            DLOGR("fudge") << std::endl;
        }
    }

    fudge::atlas::release_fonts();

    DLOGR("fudge") << "--------------------------------------------------------------------------------" << std::endl;
    DLOGR("fudge") << std::endl;

    // Configure texture map specs
    if(fudge::texmap::configure(s_tmap_config_path))
    {
        DLOGR("fudge") << std::endl;
        
        // * For each sub-directory in upack directory, create a .tom file containing every image in it,
        //   whose name is the sub-directory name
        DLOGN("fudge") << "Iterating unpacked texture maps directories." << std::endl;
        for(auto& entry: fs::directory_iterator(s_tmap_upack_path))
        {
            if(entry.is_directory())
            {
                DLOG("fudge",1) << "Processing directory: " << WCC('p') << entry.path().stem() << std::endl;

                fudge::texmap::make_tom(entry.path(), s_tmap_upack_path.parent_path());
            }
        }
    }
    else
    {
        DLOGE("fudge") << "Failed to configure texture maps." << std::endl;
    }

    return 0;
}