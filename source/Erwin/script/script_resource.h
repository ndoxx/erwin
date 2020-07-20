#pragma once
#include <string>
#include "core/hashstr.h"

namespace erwin
{

struct ScriptResource
{
	std::string entry_point;
	hash_t resource_id;
};


} // namespace erwin