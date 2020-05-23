#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace erwin
{

class SceneLoader
{
public:
	// TMP: scene loader will take a path to a scene file
	static void load_scene_stub(const fs::path& materials_path, const fs::path& hdrs_path);
	static void clear_scene();
};


} // namespace erwin