#pragma once

#include "core/registry.h"
#include "filesystem/wpath.h"

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
    erwin::WPath project_file;
    erwin::WPath root_folder;
    erwin::Registry registry;
    bool loaded = false;
};

bool load_project(const erwin::WPath& filepath);
bool save_project();
bool is_loaded();
void close_project();
const ProjectSettings& get_project_settings();

erwin::WPath asset_dir(DK dir_key);

} // namespace project
} // namespace editor