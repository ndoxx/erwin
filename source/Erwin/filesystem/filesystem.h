#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include "core/core.h"
#include "memory/memory.hpp"
#include "memory/linear_allocator.h"

namespace fs = std::filesystem;

namespace erwin
{
namespace memory
{
class HeapArea;
} // namespace memory

typedef memory::MemoryArena<memory::LinearAllocator, 
		    				memory::policy::MultiThread<std::mutex>, //memory::policy::SingleThread, 
		    				memory::policy::NoBoundsChecking,
		    				memory::policy::NoMemoryTagging,
		    				memory::policy::NoMemoryTracking> ResourceArena;

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

// Setup a linear memory arena for resources to be put on
void init_arena(memory::HeapArea& area, std::size_t size);
// Get the resource arena
ResourceArena& get_arena();
// Check that the resource arena is initialized
bool is_arena_initialized();
// Reset the resource arena
void reset_arena();

} // namespace filesystem
} // namespace erwin