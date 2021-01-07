#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace erwin
{
namespace theme
{

struct ThemeEntry
{
    std::string name;
    std::string path;
};

extern void init();
extern const std::vector<ThemeEntry>& get_list();
extern bool load(const ThemeEntry& entry);
extern bool load_default();
extern void reset();

} // namespace theme
} // namespace erwin