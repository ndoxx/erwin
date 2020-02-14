#include "entity/reflection.h"
#include <map>

namespace erwin
{

// Associate a component type ID to a reflection hash string
static std::map<uint64_t, uint64_t> s_reflection_map;

void add_reflection(uint64_t type_id, uint64_t reflected_type)
{
	s_reflection_map.insert({type_id, reflected_type});
}

uint64_t reflect(uint64_t type_id)
{
	auto it = s_reflection_map.find(type_id);
	if(it != s_reflection_map.end())
		return it->second;
	return 0;
}

} // namespace erwin