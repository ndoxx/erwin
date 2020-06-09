#include "entity/reflection.h"
#include <string>

namespace erwin
{

// Associate a component type ID to a reflection hash string
static std::map<uint64_t, uint32_t> s_reflection_map;
static std::map<uint32_t, std::string> s_component_names;

void add_reflection(uint64_t type_id, uint32_t reflected_type)
{
	s_reflection_map.insert({type_id, reflected_type});
	s_component_names.insert({reflected_type, entt::resolve_id(reflected_type).prop("name"_hs).value().cast<const char*>()});
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