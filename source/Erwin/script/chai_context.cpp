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
	for(auto actor: actors_)
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

ActorIndex ChaiContext::instantiate(const std::string& entry_point, EntityID e)
{
    DLOGN("script") << "Instantiating actor class '" << WCC('n') << entry_point << WCC(0) << "' for entity ["
                      << size_t(e) << "]" << std::endl;

    // Instantiate script object
    auto instance_handle = s_storage.actor_handle_pool_.acquire();
    s_storage.actor_instances_[instance_handle] = vm->eval(entry_point + "(" + std::to_string(int(e)) + ")");
    Actor actor;
    actor.instance_handle = instance_handle;

    // * Detect special methods
    DLOG("script",1) << "Methods: " << std::endl;
    // Update
    try
    {
        auto func = vm->eval<std::function<void(chaiscript::Boxed_Value&, float)>>("__update");
        // actor.update = std::bind(func, std::placeholders::_1, s_storage.actor_instances_.at(instance_handle)); // DNW
        actor.update = [instance_handle, func](float dt) { func(s_storage.actor_instances_.at(instance_handle), dt); };
        actor.set_trait(Actor::ActorTrait::UPDATER);
    	DLOGI << WCC('v') << "__update(float dt)" << std::endl;
    }
    catch(const chaiscript::exception::eval_error&) {}

    if(actor.traits == 0)
    {
    	DLOGI << "None" << std::endl;
    }

    actors_.push_back(actor);

    return actors_.size() - 1;
}

} // namespace script
} // namespace erwin
