#pragma once

#include <filesystem>
#include <fstream>
#include "core/wtypes.h"

namespace fs = std::filesystem;

namespace erwin
{
namespace filesystem
{

// Locate executable, and deduce paths
extern void init();

// Get executable directory
const fs::path& get_self_dir();
// Get root directory
const fs::path& get_root_dir();
// Get config directory
const fs::path& get_config_dir();
// Get application asset directory if any
const fs::path& get_asset_dir();
// Get system asset directory
const fs::path& get_system_asset_dir();
// Set application asset directory
void set_asset_dir(const fs::path& path);

// Get a stream to an asset in asset directory
std::ifstream get_asset_stream(const fs::path& path);
// Get a text file in asset directory as a string
std::string get_asset_string(const fs::path& path);

// Get a text file as a string
std::string get_file_as_string(const fs::path& path);
// Get a binary file as a vector of bytes
void get_file_as_vector(const fs::path& filepath, std::vector<uint8_t>& vec);
// Get a stream from a binary file
std::ifstream binary_stream(const fs::path& path);

} // namespace filesystem
} // namespace erwin