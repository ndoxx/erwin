#pragma once
#include <filesystem>

namespace fs = std::filesystem;

namespace editor
{

extern bool load_theme(const fs::path& xml_path);

} // namespace editor