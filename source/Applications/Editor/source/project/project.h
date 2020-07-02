#pragma once

#include "core/registry.h"
#include "filesystem/file_path.h"

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

enum class DirKey: uint8_t
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

bool load_project(const erwin::FilePath& filepath);
bool save_project();
void close_project();
const ProjectSettings& get_project_settings();

erwin::FilePath asset_dir(DirKey dir_key);
erwin::FilePath asset_path(DirKey dir_key, const fs::path& file_path);

} // namespace project
} // namespace editor