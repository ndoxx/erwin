#pragma once

#include <map>
#include <string>

#include "glm/glm.hpp"
#include "core/core.h"
#include "filesystem/xml_file.h"
#include "filesystem/wpath.h"


namespace erwin
{

class Registry
{
public:
	void deserialize(const xml::XMLFile& xml, const std::string& root_name="");
	void serialize(xml::XMLFile& xml, const std::string& root_name="");
	void clear();

	template <typename T>
	const T& get(hash_t hname, const T& def) const;
	const WPath& get(hash_t hname) const;
	bool is(hash_t name) const;
	template <typename T>
	bool set(hash_t hname, const T& value);

protected:
	hash_t parse_xml_property(void* node, const std::string& name_chain, const fs::path& parent_dir);
	void parse_properties(void* node, const std::string& name_chain, const fs::path& parent_dir);
	void write_xml_property(void* pdoc, void* node, const std::string& name_chain, const fs::path& parent_dir);
	void serialize_properties(void* pdoc, void* node, const std::string& name_chain, const fs::path& parent_dir);

private:
	std::map<hash_t, size_t>      sizes_;
	std::map<hash_t, uint32_t>    uints_;
	std::map<hash_t, int32_t>     ints_;
	std::map<hash_t, float>       floats_;
	std::map<hash_t, bool>        bools_;
	std::map<hash_t, std::string> strings_;
	std::map<hash_t, glm::vec2>   vec2s_;
	std::map<hash_t, glm::vec3>   vec3s_;
	std::map<hash_t, glm::vec4>   vec4s_;
	std::map<hash_t, WPath>       paths_;
};

template <> const size_t&      Registry::get(hash_t hname, const size_t& def) const;
template <> const uint32_t&    Registry::get(hash_t hname, const uint32_t& def) const;
template <> const int32_t&     Registry::get(hash_t hname, const int32_t& def) const;
template <> const float&       Registry::get(hash_t hname, const float& def) const;
template <> const bool&        Registry::get(hash_t hname, const bool& def) const;
template <> const std::string& Registry::get(hash_t hname, const std::string& def) const;
template <> const glm::vec2&   Registry::get(hash_t hname, const glm::vec2& def) const;
template <> const glm::vec3&   Registry::get(hash_t hname, const glm::vec3& def) const;
template <> const glm::vec4&   Registry::get(hash_t hname, const glm::vec4& def) const;

template <> bool Registry::set(hash_t hname, const size_t& val);
template <> bool Registry::set(hash_t hname, const uint32_t& val);
template <> bool Registry::set(hash_t hname, const int32_t& val);
template <> bool Registry::set(hash_t hname, const float& val);
template <> bool Registry::set(hash_t hname, const bool& val);
template <> bool Registry::set(hash_t hname, const std::string& val);
template <> bool Registry::set(hash_t hname, const WPath& val);
template <> bool Registry::set(hash_t hname, const glm::vec2& val);
template <> bool Registry::set(hash_t hname, const glm::vec3& val);
template <> bool Registry::set(hash_t hname, const glm::vec4& val);

} // namespace erwin