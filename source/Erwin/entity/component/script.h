#pragma once

#include "filesystem/wpath.h"
#include "script/script_engine.h"
#include <string>

namespace erwin
{

struct ComponentScript
{
public:
    ComponentScript() = default;
    explicit ComponentScript(const std::string& universal_path) : file_path(universal_path) {}
    explicit ComponentScript(const WPath& path) : file_path(path) {}

    WPath file_path;
    std::string entry_point;
    script::ActorIndex actor_index = 0;
    script::VMHandle script_context = 0;
};

} // namespace erwin