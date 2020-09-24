#pragma once

#include "script/chai_context.h"
#include <memory>

namespace erwin
{
namespace script
{

std::shared_ptr<chaiscript::Module> make_glm_bindings();

}
} // namespace erwin