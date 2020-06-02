#pragma once

#include <filesystem>
#include <istream>
#include <ostream>
#include <memory>
#include <type_traits>
#include "core/core.h"

namespace fs = std::filesystem;

namespace erwin
{
namespace wfs
{

enum FileMode: uint8_t
{
	ascii,
	binary,
};

// Locate executable, and deduce paths
extern void init();

// Get user directory
const fs::path& get_user_dir();
// Get executable directory
const fs::path& get_self_dir();
// Get root directory
const fs::path& get_root_dir();
// Get config directory
const fs::path& get_config_dir();
// Get application configuration directory
const fs::path& get_client_config_dir();
// Get application asset directory if any
const fs::path& get_asset_dir();
// Get system asset directory
const fs::path& get_system_asset_dir();
// Set application asset directory
void set_asset_dir(const fs::path& path);
// Set application configuration directory
void set_client_config_dir(const fs::path& path);

// Make sure a user config file exists, copy default config to user dir if necessary
bool ensure_user_config(const fs::path& user_path, const fs::path& default_path);

// Get a text file as a string
std::string get_file_as_string(const fs::path& path);
// Get a binary file as a vector of bytes
std::vector<uint8_t> get_file_as_vector(const fs::path& path);
// Get an input stream from a file
std::shared_ptr<std::istream> get_istream(const fs::path& path, uint8_t mode);
// Get an output stream to a file
std::shared_ptr<std::ostream> get_ostream(const fs::path& path, uint8_t mode);

} // namespace wfs

// Helpers for stream read/write pointer cast
// Only well defined for PODs
template <typename T, typename = std::enable_if_t<std::is_pod_v<T>>>
static inline char* opaque_cast(T* in) { return static_cast<char*>(static_cast<void*>(in)); }

template <typename T, typename = std::enable_if_t<std::is_pod_v<T>>>
static inline const char* opaque_cast(const T* in) { return static_cast<const char*>(static_cast<void*>(in)); }

} // namespace erwin