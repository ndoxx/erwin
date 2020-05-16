#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace editor
{
namespace project
{

struct ProjectSettings
{
    fs::path project_file;
};

extern bool load_project(const fs::path& filepath);
extern bool save_project();
extern const ProjectSettings& get_project_settings();

} // namespace project
} // namespace editor