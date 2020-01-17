#include "filesystem/filesystem.h"
#include "core/core.h"
#include "memory/heap_area.h"
#include "render/shader_lang.h"

#include <iterator>

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
static fs::path s_client_config_path;
static fs::path s_sys_asset_path;

static ResourceArena s_arena;

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

std::ifstream get_asset_stream(const fs::path& path)
{
    W_ASSERT(fs::exists(path), "File does not exist.");

	return std::ifstream(s_asset_path / path);
}

std::string get_asset_string(const fs::path& path)
{
    W_ASSERT(fs::exists(path), "File does not exist.");

    std::ifstream ifs(s_asset_path / path);
    return std::string((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());
}

std::string get_file_as_string(const fs::path& path)
{
    W_ASSERT(fs::exists(path), "File does not exist.");

    std::ifstream ifs(path);
    return std::string((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());
}

std::ifstream binary_stream(const fs::path& path)
{
    W_ASSERT(fs::exists(path), "File does not exist.");
    
    return std::ifstream(path, std::ios::binary);
}

void get_file_as_vector(const fs::path& filepath, std::vector<uint8_t>& vec)
{
    W_ASSERT(fs::exists(filepath), "File does not exist.");

    std::ifstream ifs(filepath, std::ios::binary);
    ifs.unsetf(std::ios::skipws);

    std::streampos file_size;

    // Get size
    ifs.seekg(0, std::ios::end);
    file_size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    // Allocate & read
    vec.reserve(file_size);
    vec.insert(vec.begin(),
               std::istream_iterator<uint8_t>(ifs),
               std::istream_iterator<uint8_t>());
}

void init_arena(memory::HeapArea& area, std::size_t size)
{
    s_arena.init(area, size, "ResourceArena");
}

ResourceArena& get_arena()
{
    W_ASSERT(s_arena.is_initialized(), "Resource arena is not initialized. Call filesystem::init_arena() first.");
    return s_arena;
}

bool is_arena_initialized()
{
    return s_arena.is_initialized();
}

void reset_arena()
{
    s_arena.reset();
}


} // namespace filesystem
} // namespace erwin