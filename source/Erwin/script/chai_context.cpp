#include "debug/logger.h"
#include "script/script_engine.h"
#include "utils/sparse_set.hpp"
#include <chaiscript/chaiscript.hpp>

namespace erwin
{
namespace script
{

static struct
{
    std::array<chaiscript::Boxed_Value, k_max_actors> actor_instances_;
    SparsePool<InstanceHandle, k_max_actors> actor_handle_pool_;
} s_storage;

ChaiContext::~ChaiContext()
{
    for(auto actor : actors_)
    {
        s_storage.actor_instances_[actor.instance_handle] = {};
        s_storage.actor_handle_pool_.release(actor.instance_handle);
    }
}

void ChaiContext::init() { vm = std::make_shared<chaiscript::ChaiScript>(); }

void ChaiContext::add_bindings(std::shared_ptr<chaiscript::Module> module) { vm->add(module); }

void ChaiContext::use(const WPath& script_path)
{
    try
    {
        vm->use(script_path.string());
    }
    catch(const chaiscript::exception::eval_error& e)
    {
        DLOGE("script") << e.pretty_print() << std::endl;
    }
}

void ChaiContext::eval(const std::string& command)
{
    try
    {
        vm->eval(command);
    }
    catch(const chaiscript::exception::eval_error& e)
    {
        DLOGE("script") << e.pretty_print() << std::endl;
    }
}

/*
    Helper function to detect a script actor method and bind it to an actor object functor.
    The functor wraps the function call in such a way that if a script error occurs during
    functor execution, the actor will be disabled.
*/
template <typename... ArgsT>
static void try_load(ChaiContext::VM_ptr vm, const std::string& name, Actor& actor,
                     std::function<void(ArgsT...)>& target_functor, Actor::ActorTrait trait)
{
    // Actor disabled on script error.
    // This behavior is passed as a lambda to target functor as capturing actor
    // by ref would not work.
    auto disable_on_fault = [&actor]() {
        actor.enable(false);
        DLOGW("script") << "Actor was disabled due to a script error." << std::endl;
    };

    try
    {
        auto func = vm->eval<std::function<void(chaiscript::Boxed_Value&, ArgsT...)>>(name);
        target_functor = [instance_handle = actor.instance_handle, func, disable_on_fault](ArgsT... args) {
            try
            {
                func(s_storage.actor_instances_.at(instance_handle), std::forward<ArgsT>(args)...);
            }
            catch(const chaiscript::exception::eval_error& e)
            {
                DLOGE("script") << e.pretty_print() << std::endl;
                disable_on_fault();
            }
        };
        actor.set_trait(trait);
        DLOGI << WCC('v') << name << std::endl;
    }
    catch(const chaiscript::exception::eval_error&)
    {
    }
}

ActorIndex ChaiContext::instantiate(const std::string& entry_point, EntityID e)
{
    DLOGN("script") << "Instantiating actor class '" << WCC('n') << entry_point << WCC(0) << "' for entity ["
                    << size_t(e) << "]" << std::endl;

    // Instantiate script object
    auto instance_handle = s_storage.actor_handle_pool_.acquire();
    DLOGI << "Instance handle: " << instance_handle << std::endl;

    s_storage.actor_instances_[instance_handle] = vm->eval(entry_point + "(" + std::to_string(int(e)) + ")");
    actors_.emplace_back(instance_handle);
    auto& actor = actors_.back();

    // * Detect special methods
    DLOG("script", 1) << "Methods: " << std::endl;
    try_load(vm, "__update", actor, actor.update, Actor::ActorTrait::UPDATER);

    if(actor.traits == 0)
    {
        DLOGI << "None" << std::endl;
    }

    return actors_.size() - 1;
}

} // namespace script
} // namespace erwin
