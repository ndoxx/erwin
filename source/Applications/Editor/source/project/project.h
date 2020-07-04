#pragma once

#include "core/registry.h"
#include "filesystem/file_path.h"

namespace editor
{

enum class DK: uint8_t
{
	ATLAS,
	HDR,
	MATERIAL,
	FONT,
	MESH,
	SCENE,

	WORK_ATLAS,
	WORK_HDR,
	WORK_MATERIAL,
	WORK_FONT,
	WORK_MESH,
	WORK_SCENE
};

namespace project
{

struct ProjectSettings
{
    fs::path project_file;
    fs::path root_folder;
    erwin::Registry registry;
    bool loaded = false;
};

bool load_project(const erwin::FilePath& filepath);
bool save_project();
bool is_loaded();
void close_project();
const ProjectSettings& get_project_settings();

erwin::FilePath asset_dir(DK dir_key);
erwin::FilePath asset_path(DK dir_key, const fs::path& file_path);

} // namespace project
} // namespace editor