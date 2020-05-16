#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace editor
{
namespace project
{

extern bool load_global_settings();
extern bool save_global_settings();
extern void set_current_project_file(const fs::path& filepath);


struct ProjectSettings
{
    fs::path project_file;
};

extern bool load_project(const fs::path& filepath);
extern bool save_project();
extern const ProjectSettings& get_project_settings();

} // namespace project
} // namespace editor