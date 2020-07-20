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

class ScriptEngine
{
public:
	VMHandle create_context();
	void destroy_context(VMHandle handle);
	inline ChaiContext& get_context(VMHandle handle) { return vms_.at(handle); }

private:
	SparsePool<VMHandle, k_max_script_vms> vm_handle_pool_;
	std::array<ChaiContext, k_max_script_vms> vms_;
};

} // namespace script
} // namespace erwin