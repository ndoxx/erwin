#include "script/script_engine.h"
#include "debug/logger.h"
#include "script/bindings/glm.h"
#include "script/bindings/logger.h"
#include "script/bindings/scene.h"
#include "script/bindings/scene_proxy.h"
#include "level/scene.h"
#include <chaiscript/chaiscript.hpp>
#include <glm/glm.hpp>

namespace erwin
{

using namespace script;

static struct
{
    SparsePool<VMHandle, k_max_script_vms> vm_handle_pool_;
    std::array<ChaiContext, k_max_script_vms> vms_;
} s_storage;

VMHandle ScriptEngine::create_context(Scene& scene)
{
    // Initialize new virtual machine, add common bindings and return handle
    auto handle = s_storage.vm_handle_pool_.acquire();
    s_storage.vms_[handle].init(handle);
    s_storage.vms_[handle].add_bindings(script::make_logger_bindings());
    s_storage.vms_[handle].add_bindings(script::make_glm_bindings());
    s_storage.vms_[handle].add_bindings(script::make_scene_bindings());

    s_storage.vms_[handle]->add_global(chaiscript::var(SceneProxy(scene)), "scene");

    // Redirect the print function
    s_storage.vms_[handle].eval("global print = DLOG;");

    DLOG("script", 1) << "Created new script context [" << WCC('n') << handle << WCC(0) << "]" << std::endl;
    return handle;
}

void ScriptEngine::destroy_context(VMHandle handle)
{
    s_storage.vms_[handle] = {};
    s_storage.vm_handle_pool_.release(handle);
    DLOG("script", 1) << "Destroyed script context [" << WCC('n') << handle << WCC(0) << "]" << std::endl;
}

ChaiContext& ScriptEngine::get_context(VMHandle handle) { return s_storage.vms_.at(handle); }

void ScriptEngine::transport_runtime_parameters(script::VMHandle source, script::VMHandle target)
{
    s_storage.vms_.at(target).update_parameters(s_storage.vms_.at(source));
}


} // namespace erwin
