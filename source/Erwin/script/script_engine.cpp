#include "script/script_engine.h"
#include "script/bindings/glm.h"
#include <chaiscript/chaiscript.hpp>

namespace erwin
{
namespace script
{

VMHandle ScriptEngine::create_context()
{
	// Initialize new virtual machine, add common bindings and return handle
	auto handle = vm_handle_pool_.acquire();
	vms_[handle].init();
	vms_[handle].add_bindings(script::make_glm_bindings());
	return handle;
}

void ScriptEngine::destroy_context(VMHandle handle)
{
	vms_[handle] = {};
	vm_handle_pool_.release(handle);
}


} // namespace script
} // namespace erwin
