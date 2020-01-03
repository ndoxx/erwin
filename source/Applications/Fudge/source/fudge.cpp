#include <fstream>
#include <sstream>
#include <string>

#ifdef __linux__
    #include <unistd.h>
    #include <climits>
#elif _WIN32

#endif

#include "asset_registry.h"
#include "atlas_packer.h"
#include "texture_packer.h"
#include "shader_packer.h"
#include "debug/logger.h"
#include "debug/logger_sink.h"
#include "debug/logger_thread.h"
#include "filesystem/xml_file.h"

using namespace erwin;

static fs::path s_self_path;  // Path to executable
static fs::path s_root_path;  // Path to root directory of Erwin engine
static fs::path s_conf_path;  // Path to configuration directory
static bool     s_force_rebuild = false; // If set to true, all assets will be rebuilt, disregarding the asset registry content
static bool     s_force_tom_rebuild = false; // If set to true, all TOM files will be rebuilt, disregarding the asset registry content
static bool     s_force_cat_rebuild = false; // If set to true, all CAT files will be rebuilt, disregarding the asset registry content
static bool     s_force_font_rebuild = false; // If set to true, all font files will be rebuilt, disregarding the asset registry content
static bool     s_force_shader_rebuild = false; // If set to true, all shader files will be rebuilt, disregarding the asset registry content

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
    WLOGGER(create_channel("fudge", 3));
    WLOGGER(attach_all("ConsoleSink", std::make_unique<dbg::ConsoleSink>()));
    WLOGGER(attach_all("MainFileSink", std::make_unique<dbg::LogFileSink>("fudge.log")));
    WLOGGER(set_single_threaded(true));
}

static bool cmd_option_exists(const char** begin, const char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

int main(int argc, char const *argv[])
{
    init_logger();
    show_logo();

    // Force rebuild
    s_force_rebuild        = cmd_option_exists(argv, argv + argc, "-f") || cmd_option_exists(argv, argv + argc, "--force");
    s_force_tom_rebuild    = s_force_rebuild || cmd_option_exists(argv, argv + argc, "--ftom");
    s_force_cat_rebuild    = s_force_rebuild || cmd_option_exists(argv, argv + argc, "--fcat");
    s_force_font_rebuild   = s_force_rebuild || cmd_option_exists(argv, argv + argc, "--ffont");
    s_force_shader_rebuild = s_force_rebuild || cmd_option_exists(argv, argv + argc, "--fshader");

    // Test
    if(cmd_option_exists(argv, argv + argc, "-t"))
    {
        fudge::spv::test();
        return 0;
    }

    // * Locate executable path, root directory, config directory, asset and fonts directories
    DLOGN("fudge") << "Locating unpacked assets." << std::endl;
    s_self_path = get_selfpath();
    s_root_path = s_self_path.parent_path().parent_path();
    s_conf_path = s_root_path / "config";

    DLOGI << "Self path: " << WCC('p') << s_self_path << std::endl;
    DLOGI << "Root path: " << WCC('p') << s_root_path << std::endl;
    DLOGI << "Conf path: " << WCC('p') << s_conf_path << std::endl;

    xml::XMLFile cfg(s_conf_path / "fudge.xml");
    if(!cfg.read())
    {
        DLOGE("fudge") << "Could not complete configuration step." << std::endl;
        DLOGI << "Missing file: " << WCC('p') << (s_conf_path / "fudge.xml") << std::endl;
        DLOGI << "Exiting." << std::endl;
        exit(EXIT_FAILURE);
    }
    // Load asset registry file
    fudge::far::load(s_conf_path / "fudge.far");

    DLOGR("fudge") << std::endl;
    DLOGR("fudge") << "--------------------------------------------------------------------------------" << std::endl;
    DLOGR("fudge") << std::endl;

    // ---------------- TEXTURE ATLASES ----------------
    rapidxml::xml_node<>* atl_node = cfg.root->first_node("atlas");
    if(atl_node)
    {
        rapidxml::xml_node<>* tatl_node = atl_node->first_node("texture");
        if(tatl_node)
        {
            for(rapidxml::xml_node<>* batch=tatl_node->first_node("batch");
                batch; batch=batch->next_sibling("batch"))
            {
                // Configure batch
                std::string input_path, output_path, tex_compression("none"), blob_compression("none");
                if(!xml::parse_attribute(batch, "input", input_path)) continue;
                if(!xml::parse_attribute(batch, "output", output_path)) continue;
                xml::parse_node(batch, "texture_compression", tex_compression);
                xml::parse_node(batch, "blob_compression", blob_compression);

                fudge::Compression tex_c;
                switch(H_(tex_compression.c_str()))
                {
                    case "DXT5"_h: tex_c = fudge::Compression::DXT5; break;
                    default:       tex_c = fudge::Compression::None; break;
                }
                fudge::Compression blob_c;
                switch(H_(tex_compression.c_str()))
                {
                    case "deflate"_h: blob_c = fudge::Compression::Deflate; break;
                    default:          blob_c = fudge::Compression::None; break;
                }
                fudge::atlas::set_compression(blob_c);

                // Iterate directory
                DLOGN("fudge") << "Iterating unpacked atlases directory:" << std::endl;
                DLOGI << WCC('p') << input_path << WCC(0) << std::endl;
                for(auto& entry: fs::directory_iterator(s_root_path / input_path))
                {
                    if(entry.is_directory() && (fudge::far::need_create(entry) || s_force_cat_rebuild))
                    {
                        DLOG("fudge",1) << "Processing directory: " << WCC('p') << entry.path().stem() << std::endl;

                        fudge::atlas::make_atlas(entry.path(), s_root_path / output_path, tex_c);
                        DLOGR("fudge") << std::endl;
                    }
                }
            }

            DLOGR("fudge") << "--------------------------------------------------------------------------------" << std::endl;
            DLOGR("fudge") << std::endl;
        }
        else
        {
            DLOGW("fudge") << "Cannot find \"texture\" node. Skipping texture atlas packing." << std::endl;
        }

        // ---------------- FONT ATLASES ----------------
        rapidxml::xml_node<>* fatl_node = atl_node->first_node("font");
        if(fatl_node)
        {
            fudge::atlas::init_fonts();

            for(rapidxml::xml_node<>* batch=fatl_node->first_node("batch");
                batch; batch=batch->next_sibling("batch"))
            {
                // Configure batch
                std::string input_path, output_path;
                if(!xml::parse_attribute(batch, "input", input_path)) continue;
                if(!xml::parse_attribute(batch, "output", output_path)) continue;

                DLOGN("fudge") << "Iterating fonts directory:" << std::endl;
                DLOGI << WCC('p') << input_path << WCC(0) << std::endl;
                for(auto& entry: fs::directory_iterator(s_root_path / input_path))
                {
                    if(entry.is_regular_file() && 
                       !entry.path().extension().string().compare(".ttf") &&
                       (fudge::far::need_create(entry) || s_force_font_rebuild))
                    {
                        DLOG("fudge",1) << "Processing font: " << WCC('n') << entry.path().filename() << std::endl;
                        fudge::atlas::make_font_atlas(entry.path(), s_root_path / output_path, fudge::Compression::None, 32);
                        DLOGR("fudge") << std::endl;
                    }
                }
            }

            fudge::atlas::release_fonts();

            DLOGR("fudge") << "--------------------------------------------------------------------------------" << std::endl;
            DLOGR("fudge") << std::endl;
        }
        else
        {
            DLOGW("fudge") << "Cannot find \"font\" node. Skipping font atlas packing." << std::endl;
        }
    }
    else
    {
        DLOGW("fudge") << "Cannot find \"atlas\" node. Skipping atlas packing." << std::endl;
    }

    // ---------------- TEXTURE MAPS ----------------
    rapidxml::xml_node<>* tm_node = cfg.root->first_node("texmap");
    if(tm_node)
    {
        for(rapidxml::xml_node<>* batch=tm_node->first_node("batch");
            batch; batch=batch->next_sibling("batch"))
        {
            // Configure batch
            std::string input_path, output_path, config_file;
            if(!xml::parse_attribute(batch, "input", input_path)) continue;
            if(!xml::parse_attribute(batch, "output", output_path)) continue;
            if(!xml::parse_node(batch, "config", config_file)) continue;
            if(!fudge::texmap::configure(s_root_path / config_file))
            {
                DLOGE("fudge") << "Failed to configure texture maps." << std::endl;
                continue;
            }

            DLOGN("fudge") << "Iterating unpacked texture maps directory:" << std::endl;
            DLOGI << WCC('p') << input_path << WCC(0) << std::endl;
            for(auto& entry: fs::directory_iterator(s_root_path / input_path))
                if(entry.is_directory() && (fudge::far::need_create(entry) || s_force_tom_rebuild))
                    fudge::texmap::make_tom(entry.path(), s_root_path / output_path);
        }

        DLOGR("fudge") << "--------------------------------------------------------------------------------" << std::endl;
        DLOGR("fudge") << std::endl;
    }
    else
    {
        DLOGW("fudge") << "Cannot find \"texmap\" node. Skipping texture maps packing." << std::endl;
    }

    // ---------------- SHADERS ----------------
    if(fudge::spv::check_toolchain())
    {
        rapidxml::xml_node<>* shader_node = cfg.root->first_node("shader");
        for(rapidxml::xml_node<>* batch=shader_node->first_node("batch");
            batch; batch=batch->next_sibling("batch"))
        {
            // Configure batch
            std::string input_path, output_path, config_file;
            if(!xml::parse_attribute(batch, "input", input_path)) continue;
            if(!xml::parse_attribute(batch, "output", output_path)) continue;

            // Create temporary folder
            fs::create_directory(s_root_path / output_path / "tmp");
            DLOGN("fudge") << "Iterating shaders directory:" << std::endl;
            DLOGI << WCC('p') << input_path << WCC(0) << std::endl;
            for(auto& entry: fs::directory_iterator(s_root_path / input_path))
            {
                if(entry.is_regular_file() && 
                   !entry.path().extension().string().compare(".glsl") &&
                   (fudge::far::need_create(entry) || s_force_shader_rebuild))
                {
                    DLOG("fudge",1) << "Processing: " << WCC('n') << entry.path().filename() << WCC(0) << std::endl;
                    fudge::spv::make_shader_spirv(entry.path(), s_root_path / output_path);
                    DLOGR("fudge") << std::endl;
                }
            }
            // Delete temporary folder
            fs::remove_all(s_root_path / output_path / "tmp");
        }
    }

    // Save asset registry file
    fudge::far::save(s_conf_path / "fudge.far");

    return 0;
}