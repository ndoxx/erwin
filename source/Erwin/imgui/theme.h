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

extern void list();
extern const std::vector<ThemeEntry>& get_list();
extern bool load(const ThemeEntry& entry);
extern bool load_by_name(erwin::hash_t name);

} // namespace theme
} // namespace editor