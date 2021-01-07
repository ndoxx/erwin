#pragma once

#include "core/registry.h"


namespace editor
{

enum class DK: uint8_t
{
	ATLAS,
	HDR,
	MATERIAL,
	FONT,
	MESH,
	SCRIPT,
	SCENE,

	WORK_ATLAS,
	WORK_HDR,
	WORK_MATERIAL,
	WORK_FONT,
	WORK_MESH,
	WORK_SCRIPT,
	WORK_SCENE,
};

namespace project
{

struct ProjectSettings
{
    erwin::std::string project_file;
    erwin::std::string root_folder;
    erwin::Registry registry;
    bool loaded = false;
};

bool load_project(const erwin::std::string& filepath);
bool save_project();
bool is_loaded();
void close_project();
const ProjectSettings& get_project_settings();

erwin::std::string asset_dir(DK dir_key);

} // namespace project
} // namespace editor