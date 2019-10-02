#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace fudge
{
namespace far
{

void load(const fs::path& registry_file_path);

void save(const fs::path& registry_file_path);

bool need_create(const fs::directory_entry& entry);

} // namespace far
} // namespace fudge