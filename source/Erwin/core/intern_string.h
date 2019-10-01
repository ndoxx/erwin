#pragma once

#include <string>

#include "core/file_system.h"

namespace erwin
{
namespace istr
{

typedef unsigned long long hash_t;

// Read intern string file and initialize registry
extern void init(const fs::path& filepath);
// Add an intern string to the registry
extern void add(const std::string& str);
// Resolve an intern string to a full string
extern std::string resolve(hash_t hname);

} // namespace istr
} // namespace erwin
