#pragma once

#include "script/chai_context.h"
#include <memory>

namespace erwin
{

class Scene;

namespace script
{

std::shared_ptr<chaiscript::Module> make_scene_bindings();

}
} // namespace erwin