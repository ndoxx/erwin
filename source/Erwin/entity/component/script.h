#pragma once

#include <string>
#include "script/script_engine.h"
#include "script/script_resource.h"

namespace erwin
{

struct ComponentScript
{
	ScriptResource script;
	script::VMHandle vm;
};

} // namespace erwin