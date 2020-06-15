#pragma once

#include <string>
#include <filesystem>
#include "glm/glm.hpp"
#include "core/core.h"

namespace fs = std::filesystem;

namespace erwin
{
namespace cfg
{

// Load a config file
extern bool load(const fs::path& filepath);
// Save relevant properties to config file
extern bool save(const fs::path& filepath);

template <typename T> const T& get(hash_t hname, const T& def);
template <> const size_t&      get(hash_t hname, const size_t& def);
template <> const uint32_t&    get(hash_t hname, const uint32_t& def);
template <> const int32_t&     get(hash_t hname, const int32_t& def);
template <> const float&       get(hash_t hname, const float& def);
template <> const bool&        get(hash_t hname, const bool& def);
template <> const std::string& get(hash_t hname, const std::string& def);
template <> const glm::vec2&   get(hash_t hname, const glm::vec2& def);
template <> const glm::vec3&   get(hash_t hname, const glm::vec3& def);
template <> const glm::vec4&   get(hash_t hname, const glm::vec4& def);
const fs::path& get(hash_t hname);

template <typename T> bool set(hash_t hname, const T& val);
template <> bool set(hash_t hname, const size_t& val);
template <> bool set(hash_t hname, const uint32_t& val);
template <> bool set(hash_t hname, const int32_t& val);
template <> bool set(hash_t hname, const float& val);
template <> bool set(hash_t hname, const bool& val);
template <> bool set(hash_t hname, const std::string& val);
template <> bool set(hash_t hname, const fs::path& val);
template <> bool set(hash_t hname, const glm::vec2& val);
template <> bool set(hash_t hname, const glm::vec3& val);
template <> bool set(hash_t hname, const glm::vec4& val);

} // namespace cfg
} // namespace erwin