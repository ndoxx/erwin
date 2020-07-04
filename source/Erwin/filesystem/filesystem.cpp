#include "filesystem/filesystem.h"
#include "core/core.h"
#include "debug/logger.h"
#include "render/shader_lang.h"
#include "event/event_bus.h"
#include "event/window_events.h"

#include <iterator>
#include <fstream>

#ifdef __linux__
    #include <unistd.h>
    #include <climits>
    #include <sys/types.h>
    #include <pwd.h>
#elif _WIN32

#endif

namespace erwin
{
namespace wfs
{

static fs::path s_user_dir;
static fs::path s_self_path;
static fs::path s_conf_path;
static fs::path s_root_path;
static fs::path s_asset_path;
static fs::path s_client_config_path;
static fs::path s_sys_asset_path;


// Get path to executable
static fs::path get_selfpath()
{
#ifdef __linux__
    char buff[PATH_MAX];
    ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
    W_ASSERT(len != -1, "Cannot read self path using readlink.");

    if(len != -1)
        buff[len] = '\0';
    return fs::path(buff);
#else
    W_ASSERT(false, "get_selfpath() not yet implemented for this platform.");
#endif

    return fs::path();
}


static fs::path get_home_directory()
{
#ifdef __linux__
    const char* homedir;

    if((homedir = getenv("HOME")) == NULL)
        homedir = getpwuid(getuid())->pw_dir;

    return fs::path(homedir);
#else
    W_ASSERT(false, "get_home_directory() not yet implemented for this platform.");
#endif

    return "";
}

static void build_user_dir()
{
    if(!fs::create_directory(s_user_dir))
    {
        DLOGE("config") << "Failed to create user directory." << std::endl;
        return;
    }
    if(!fs::create_directory(s_user_dir / "config"))
    {
        DLOGE("config") << "Failed to create config directory in user directory." << std::endl;
        return;
    }

    DLOGN("config") << "User directory created:" << std::endl;
    DLOGI << WCC('p') << s_user_dir << std::endl;
}

static void check_user_dir()
{
    s_user_dir = get_home_directory() / ".erwin";

    // * Check existence (TODO: and integrity) of user directory
    if(!fs::exists(s_user_dir))
        build_user_dir();
}

void init()
{
    s_self_path = get_selfpath();
    s_root_path = s_self_path.parent_path().parent_path();
    s_conf_path = s_root_path / "config";
    W_ASSERT(fs::exists(s_conf_path), "No config directory detected in root directory.");
    s_sys_asset_path = s_root_path / "source/Erwin/assets";
    W_ASSERT(fs::exists(s_sys_asset_path), "No assets directory detected in Erwin source directory.");
    // Register shader directory as an include directory for client shaders
    slang::register_include_directory(s_sys_asset_path / "shaders");
    s_asset_path = fs::path();

    check_user_dir();
}

const fs::path& get_user_dir()
{
    return s_user_dir;
}

const fs::path& get_self_dir()
{
    return s_self_path;
}

const fs::path& get_root_dir()
{
	return s_root_path;
}

const fs::path& get_config_dir()
{
	return s_conf_path;
}

const fs::path& get_client_config_dir()
{
    return s_client_config_path;
}

const fs::path& get_asset_dir()
{
	return s_asset_path;
}

const fs::path& get_system_asset_dir()
{
    return s_sys_asset_path;
}

void set_asset_dir(const fs::path& path)
{
	s_asset_path = s_root_path / path;
}

void set_client_config_dir(const fs::path& path)
{
    s_client_config_path = s_root_path / path;
}

bool ensure_user_config(const fs::path& user_path, const fs::path& default_path)
{
    bool has_user    = fs::exists(user_path);
    bool has_default = fs::exists(default_path);

    if(!has_default && !has_user)
    {
        DLOGE("config") << "Failed to open user and default files:" << std::endl;
        DLOGI << "User:    " << WCC('p') << user_path << std::endl;
        DLOGI << "Default: " << WCC('p') << default_path << std::endl;
        return false;
    }

    bool copy_default = (has_default && !has_user);
    if(has_default && has_user)
    {
        // Copy default if more recent
        auto ftime_u         = fs::last_write_time(user_path);
        std::time_t cftime_u = decltype(ftime_u)::clock::to_time_t(ftime_u);
        auto ftime_d         = fs::last_write_time(default_path);
        std::time_t cftime_d = decltype(ftime_d)::clock::to_time_t(ftime_d);
        copy_default = (cftime_d > cftime_u);
    }

    if(copy_default)
    {
        DLOG("config",1) << "Copying default config:" << std::endl;
        DLOGI << "User:    " << WCC('p') << user_path << std::endl;
        DLOGI << "Default: " << WCC('p') << default_path << std::endl;
        fs::copy_file(default_path, user_path, fs::copy_options::overwrite_existing);
    }

    return true;
}

std::string get_file_as_string(const FilePath& path)
{
    W_ASSERT(path.exists(), "File does not exist.");

    std::ifstream ifs(path.full_path());
    return std::string((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());
}

std::vector<uint8_t> get_file_as_vector(const FilePath& filepath)
{
    W_ASSERT(filepath.exists(), "File does not exist.");

    std::ifstream ifs(filepath.full_path(), std::ios::binary);
    ifs.unsetf(std::ios::skipws);

    std::streampos file_size;

    // Get size
    ifs.seekg(0, std::ios::end);
    file_size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    // Allocate & read
    std::vector<uint8_t> vec;
    vec.reserve(size_t(file_size));
    vec.insert(vec.begin(),
               std::istream_iterator<uint8_t>(ifs),
               std::istream_iterator<uint8_t>());
    return vec;
}

std::shared_ptr<std::istream> get_istream(const FilePath& file_path, uint8_t mode)
{
    auto std_mode = std::ios::in;
    if(mode & FileMode::binary)
        std_mode |= std::ios::binary;

    // Get stream to file
    std::shared_ptr<std::ifstream> ifs = std::make_shared<std::ifstream>(file_path.full_path(), std_mode);

    // Sanity check
    if(!ifs->is_open())
    {
        DLOGE("ios") << "Unable to open input file:" << std::endl;
        DLOGI << WCC('p') << file_path << std::endl;
        return nullptr;
    }

    DLOG("ios",0) << "Getting input stream from file:" << std::endl;
    DLOGI << WCC('p') << file_path << std::endl;

    return ifs;
}

std::shared_ptr<std::ostream> get_ostream(const FilePath& file_path, uint8_t mode)
{
    auto std_mode = std::ios::out;
    if(mode & FileMode::binary)
        std_mode |= std::ios::binary;

    // Create stream to file
    std::shared_ptr<std::ofstream> ofs = std::make_shared<std::ofstream>(file_path.full_path(), std_mode);

    // Sanity check
    if(!ofs->is_open())
    {
        DLOGE("ios") << "Unable to open output file:" << std::endl;
        DLOGI << WCC('p') << file_path << std::endl;
        return nullptr;
    }

    DLOG("ios",0) << "Getting output stream to file:" << std::endl;
    DLOGI << WCC('p') << file_path << std::endl;

    return ofs;
}


} // namespace wfs
} // namespace erwin