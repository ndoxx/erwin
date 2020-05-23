#pragma once

#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

namespace editor
{
namespace dialog
{
// Simple wrapper around IGFD

void show_open_directory(const std::string& key, const char* title, const fs::path& default_path = ".");
void show_open(const std::string& key, const char* title, const char* filter, const fs::path& default_path = ".", const std::string& default_filename = "");
void on_open(const std::string& key, std::function<void(const fs::path& filepath)> visit);

} // namespace dialog
} // namespace editor