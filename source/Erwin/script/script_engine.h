#pragma once

#include <memory>
#include <array>
#include "script/chai_context.h"
#include "utils/sparse_set.hpp"

namespace erwin
{
namespace script
{

static constexpr size_t k_max_script_vms = 8;
using VMHandle = size_t;

} // namespace script

class ScriptEngine
{
public:
	static script::VMHandle create_context();
	static void destroy_context(script::VMHandle handle);
	static script::ChaiContext& get_context(script::VMHandle handle);
};

} // namespace erwin