#include "core/intern_string.h"
#include "debug/logger.h"
#include "filesystem/filesystem.h"
#include "script/script_engine.h"
#include "utils/sparse_set.hpp"
#include "entity/component/serial/script.h"
#include "utils/string.h"
#include <chaiscript/chaiscript.hpp>
#include <regex>
#include <cstdlib>

namespace erwin
{
namespace script
{

static struct
{
    std::array<chaiscript::Boxed_Value, k_max_actors> actor_instances_;
    SparsePool<InstanceHandle, k_max_actors> actor_handle_pool_;
} s_storage;

void Actor::update_parameters(const Actor& other)
{
    for(auto&& [name, pref]: floats_)
        pref.get() = other.floats_.at(name);
}

ChaiContext::~ChaiContext()
{
    for(auto actor : actors_)
    {
        s_storage.actor_instances_[actor.instance_handle] = {};
        s_storage.actor_handle_pool_.release(actor.instance_handle);
    }
}

void ChaiContext::init(VMHandle handle)
{
    vm = std::make_shared<chaiscript::ChaiScript>();
    handle_ = handle;
}

void ChaiContext::add_bindings(std::shared_ptr<chaiscript::Module> module) { vm->add(module); }

hash_t ChaiContext::use(const WPath& script_path)
{
    try
    {
        vm->use(script_path.string());

        // If opened for the first time, reflect actor class
        auto findit = used_files_.find(script_path.resource_id());
        if(findit == used_files_.end())
        {
            hash_t type = reflect(script_path);
            used_files_.insert({script_path.resource_id(), type});
            return type;
        }
        else
            return findit->second;
    }
    catch(const chaiscript::exception::eval_error& e)
    {
        DLOGE("script") << e.pretty_print() << std::endl;
    }
    return 0;
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

static std::tuple<float,float,float> make_range(const std::string& str_list)
{
    auto tokens = su::tokenize(str_list,',');
    W_ASSERT(tokens.size() == 3, "Script parameter range must match: <min,max,default>");
    return
    {
        std::strtof(tokens[0].c_str(), nullptr),
        std::strtof(tokens[1].c_str(), nullptr),
        std::strtof(tokens[2].c_str(), nullptr)
    };
}

static const std::regex actor_rx("#pragma\\s*actor\\s*(.+)");
static const std::regex param_rx("#pragma\\s*param<(.+?)>\\s*range<(.+?)>\\s*var (.+?);");
hash_t ChaiContext::reflect(const WPath& script_path)
{
    auto src = wfs::get_file_as_string(script_path);

    ActorReflection reflection;
    hash_t hname;

    // Reflect actor class name
    {
        std::smatch actor_match;
        if(std::regex_search(src, actor_match, actor_rx))
        {
            reflection.name = actor_match[1];
            hname = H_(reflection.name.c_str());
        }
    }

    // Reflect parameters
    {
        std::smatch param_match;
        std::string::const_iterator start(src.cbegin());
        while(std::regex_search(start, src.cend(), param_match, param_rx))
        {
            std::string param_name = param_match[1];
            reflection.parameters.push_back({H_(param_name.c_str()), param_match[3], make_range(param_match[2])});
            start = param_match.suffix().first;
        }
    }

    reflections_.insert({hname, std::move(reflection)});
    return hname;
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

ActorIndex ChaiContext::instantiate(hash_t actor_type, EntityID e)
{
    if(actor_type == 0)
    {
        DLOGE("script") << "Actor class cannot be instantiated due to a parsing error." << std::endl;
        return 0;
    }

    // Get type reflection
    const auto& reflection = reflections_.at(actor_type);

    DLOGN("script") << "Instantiating actor class '" << WCC('n') << reflection.name << WCC(0) << "' for entity ["
                    << size_t(e) << "]" << std::endl;

    // Instantiate script object
    auto instance_handle = s_storage.actor_handle_pool_.acquire();
    DLOGI << "Instance handle: " << instance_handle << std::endl;

    auto& instance = s_storage.actor_instances_[instance_handle] =
        vm->eval(reflection.name + "(" + std::to_string(int(e)) + ")");

    actors_.emplace_back(instance_handle);
    auto& actor = actors_.back();
    actor.actor_type = actor_type;

    // * Detect special methods
    DLOG("script", 1) << "Methods: " << std::endl;
    try_load(vm, "__update", actor, actor.update, Actor::ActorTrait::UPDATER);

    if(actor.traits == 0)
    {
        DLOGI << "None" << std::endl;
    }

    // * Share exposed parameters
    DLOG("script", 1) << "Parameter set: " << std::endl;
    for(const auto& param : reflection.parameters)
    {
        DLOGI << istr::resolve(param.type) << ' ' << WCC('n') << param.name << std::endl;
        switch(param.type)
        {
        case "float"_h: {
            auto func = vm->eval<std::function<float&(chaiscript::Boxed_Value&)>>(param.name);
            actor.add_parameter(param.name, std::ref<float>(func(instance)));
            break;
        }
        default: {
            DLOGW("script") << "Ignoring parameter '" << param.name << "' of unknown type " << istr::resolve(param.type)
                            << std::endl;
        }
        }
    }

    return actors_.size() - 1;
}

void ChaiContext::setup_component(ComponentScript& cscript, EntityID e)
{
    hash_t actor_type = use(cscript.file_path);
    cscript.actor_index = instantiate(actor_type, e);
    cscript.entry_point = get_reflection(actor_type).name;
    cscript.script_context = handle_;
}

void ChaiContext::update_parameters(const ChaiContext& other)
{
    // Transport parameter values from other VM to this one
    // ASSUME: Actors are the same in both sides
    W_ASSERT(actors_.size() == other.actors_.size(), "Actor vector size mismatch.");
    for(size_t ii=0; ii<actors_.size(); ++ii)
    {
        actors_[ii].update_parameters(other.actors_[ii]);
    }
}


#ifdef W_DEBUG
#include <fstream>
void ChaiContext::dbg_dump_state(const std::string& outfile)
{
    std::ofstream ofs(outfile);

    for(const auto& objs : vm->get_state().engine_state.m_function_objects)
    {
        auto paramList = objs.second->get_param_types();

        ofs << paramList[0].name() << " " << objs.first << "()\n";

        for(auto it = paramList.begin() + 1; it != paramList.end(); ++it)
        {
            ofs << "    -> " << it->name() << '\n';
        }
    }

    ofs << std::endl;
    ofs.close();
}
#endif

} // namespace script
} // namespace erwin
