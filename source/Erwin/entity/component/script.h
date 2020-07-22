#pragma once

#include <string>
#include "script/script_engine.h"
#include "filesystem/wpath.h"

namespace erwin
{

struct ComponentScript
{
public:
	ComponentScript() = default;
	explicit ComponentScript(const std::string& universal_path);
	explicit ComponentScript(const WPath& path);

	WPath file_path;
	std::string entry_point;
	script::ActorIndex actor_index = 0;

private:
	void detect_entry_point();
};

} // namespace erwin