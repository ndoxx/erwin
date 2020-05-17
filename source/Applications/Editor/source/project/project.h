#pragma once

#include <filesystem>
#include "core/registry.h"

namespace fs = std::filesystem;

namespace editor
{
namespace project
{

struct ProjectSettings
{
    fs::path project_file;
    fs::path root_folder;
    erwin::Registry registry;
};

extern bool load_project(const fs::path& filepath);
extern bool save_project();
extern void close_project();
extern const ProjectSettings& get_project_settings();

} // namespace project
} // namespace editor