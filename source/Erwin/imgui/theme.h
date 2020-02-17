#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include "core/core.h"

namespace fs = std::filesystem;

namespace editor
{
namespace theme
{

struct ThemeEntry
{
	std::string name;
	fs::path path;
};

extern void init();
extern const std::vector<ThemeEntry>& get_list();
extern bool load(const ThemeEntry& entry);
extern bool load_default();
extern void reset();

} // namespace theme
} // namespace editor