#pragma once

#include <string>
#include "script/script_engine.h"
#include "filesystem/wpath.h"

namespace erwin
{

struct ComponentScript
{
	ComponentScript() = default;
	explicit ComponentScript(const std::string& universal_path);

	WPath file_path;
	std::string entry_point;
};

} // namespace erwin