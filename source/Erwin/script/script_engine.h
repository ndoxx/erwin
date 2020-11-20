#pragma once

#include <memory>
#include <array>
#include "script/chai_context.h"

namespace erwin
{

class Scene;
class ScriptEngine
{
public:
	static script::VMHandle create_context(Scene& scene);
	static void destroy_context(script::VMHandle handle);
	static script::ChaiContext& get_context(script::VMHandle handle);
	static void transport_runtime_parameters(script::VMHandle source, script::VMHandle target);
};

} // namespace erwin