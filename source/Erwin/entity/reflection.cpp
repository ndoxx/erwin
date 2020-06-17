#include "entity/reflection.h"
#include <string>
#include <set>

namespace erwin
{

// Associate a component type ID to a reflection hash string
static std::map<uint64_t, uint32_t> s_reflection_map;
static std::map<uint32_t, std::string> s_component_names;
static std::set<uint32_t> s_hidden_types;

void add_reflection(uint64_t type_id, uint32_t reflected_type)
{
	s_reflection_map.insert({type_id, reflected_type});
	s_component_names.insert({reflected_type, entt::resolve_id(reflected_type).prop("name"_hs).value().cast<const char*>()});
}

// OPT: find a way to do this with meta type properties without having to
// write another specialized REFLECT_COMPONENT macro.
void hide_from_inspector(uint32_t reflected_type)
{
	s_hidden_types.insert(reflected_type);
}

bool is_hidden_from_inspector(uint32_t reflected_type)
{
	return (s_hidden_types.find(reflected_type) != s_hidden_types.end());
}

uint32_t reflect(uint64_t type_id)
{
	auto it = s_reflection_map.find(type_id);
	if(it != s_reflection_map.end())
		return it->second;
	return 0;
}

const std::map<uint32_t, std::string>& get_component_names()
{
	return s_component_names;
}


} // namespace erwin