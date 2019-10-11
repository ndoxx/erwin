#pragma once

#include <string>
#include "glm/glm.hpp"
#include "core/wtypes.h"
#include "filesystem/filesystem.h"

namespace erwin
{
namespace cfg
{

extern bool init(const fs::path& filepath);

template <typename T> T get(hash_t hname, T def);
template <> uint32_t    get(hash_t hname, uint32_t def);
template <> int32_t     get(hash_t hname, int32_t def);
template <> float       get(hash_t hname, float def);
template <> bool        get(hash_t hname, bool def);
template <> std::string get(hash_t hname, std::string def);
template <> glm::vec2   get(hash_t hname, glm::vec2 def);
template <> glm::vec3   get(hash_t hname, glm::vec3 def);
template <> glm::vec4   get(hash_t hname, glm::vec4 def);

} // namespace cfg
} // namespace erwin