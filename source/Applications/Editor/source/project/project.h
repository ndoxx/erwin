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

extern bool load_project(const erwin::FilePath& filepath);
extern bool save_project();
extern void close_project();
extern const ProjectSettings& get_project_settings();

extern fs::path get_asset_path(DirKey dir_key);

} // namespace project
} // namespace editor