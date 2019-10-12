#pragma once

#include <unordered_map>
#include <string>
#include <filesystem>

#include "glm/glm.hpp"
#include "core/wtypes.h"

namespace fs = std::filesystem;

namespace erwin
{

class ValueMap
{
public:
	ValueMap() = default;
	~ValueMap() = default;

void init(void* xml_root_node, const std::string& rootname);

template <typename T>
T get(hash_t hname, T def);
fs::path get(hash_t hname);
bool is(hash_t name);

protected:
	hash_t parse_xml_property(void* node, const std::string& name_chain);
	void parse_properties(void* node, const std::string& name_chain);

private:
	std::unordered_map<hash_t, uint32_t>    uints_;
	std::unordered_map<hash_t, int32_t>     ints_;
	std::unordered_map<hash_t, float>       floats_;
	std::unordered_map<hash_t, bool>        bools_;
	std::unordered_map<hash_t, std::string> strings_;
	std::unordered_map<hash_t, glm::vec2>   vec2s_;
	std::unordered_map<hash_t, glm::vec3>   vec3s_;
	std::unordered_map<hash_t, glm::vec4>   vec4s_;
	std::unordered_map<hash_t, fs::path>    paths_;
};

template <> uint32_t    ValueMap::get(hash_t hname, uint32_t def);
template <> int32_t     ValueMap::get(hash_t hname, int32_t def);
template <> float       ValueMap::get(hash_t hname, float def);
template <> bool        ValueMap::get(hash_t hname, bool def);
template <> std::string ValueMap::get(hash_t hname, std::string def);
template <> glm::vec2   ValueMap::get(hash_t hname, glm::vec2 def);
template <> glm::vec3   ValueMap::get(hash_t hname, glm::vec3 def);
template <> glm::vec4   ValueMap::get(hash_t hname, glm::vec4 def);

} // namespace erwin