#pragma once

#include <memory>
#include <array>
#include "script/chai_context.h"
#include "utils/sparse_set.hpp"

namespace erwin
{

class Scene;
class ScriptEngine
{
public:
	static script::VMHandle create_context(Scene& scene);
	static void destroy_context(script::VMHandle handle);
	static script::ChaiContext& get_context(script::VMHandle handle);
};

} // namespace erwin