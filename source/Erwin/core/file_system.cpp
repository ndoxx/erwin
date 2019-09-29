#include "core/file_system.h"
#include "core/core.h"
#include "debug/logger.h"

#ifdef __linux__
    #include <unistd.h>
    #include <climits>
#elif _WIN32

#endif

namespace erwin
{
namespace filesystem
{

// Get path to executable
static fs::path get_selfpath()
{
#ifdef __linux__
    char buff[PATH_MAX];
    std::size_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
    W_ASSERT(len != -1, "Cannot read self path using readlink.");

    buff[len] = '\0';
    return fs::path(buff);
#elif _WIN32
    W_ASSERT(false, "get_selfpath() not yet implemented for Windows.");
#endif
    return fs::path();
}


static fs::path s_self_path;
static fs::path s_conf_path;
static fs::path s_root_path;
static fs::path s_asset_path;

void init()
{
	DLOGN("config") << "Initializing file system." << std::endl;

    s_self_path = get_selfpath();
    s_root_path = s_self_path.parent_path().parent_path();
    s_conf_path = s_root_path / "config";
    s_asset_path = fs::path();

    DLOGI << "Executable path: " << WCC('p') << s_self_path.string() << WCC(0) << std::endl;
    DLOGI << "Root dir:        " << WCC('p') << s_root_path.string() << WCC(0) << std::endl;
    DLOGI << "Config dir:      " << WCC('p') << s_conf_path.string() << WCC(0) << std::endl;
}

const fs::path& get_root_dir()
{
	return s_root_path;
}

const fs::path& get_config_dir()
{
	return s_conf_path;
}

const fs::path& get_asset_dir()
{
	return s_asset_path;
}

void set_asset_dir(const fs::path& path)
{
	s_asset_path = s_root_path / path;
}

std::ifstream get_asset_stream(const fs::path& path)
{
	return std::ifstream(s_asset_path / path);
}

std::string get_asset_string(const fs::path& path)
{
    std::ifstream ifs(s_asset_path / path);
    return std::string((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());
}

std::string get_file_as_string(const fs::path& path)
{
    std::ifstream ifs(path);
    return std::string((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());
}


} // namespace filesystem
} // namespace erwin