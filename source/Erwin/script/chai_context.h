#pragma once

#include "core/core.h"
#include "entity/reflection.h"
#include "filesystem/wpath.h"
#include "script/common.h"
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace chaiscript
{
class ChaiScript;
class Module;
} // namespace chaiscript

namespace erwin
{
struct ComponentScript;

namespace script
{

template <typename ExposedT> std::shared_ptr<chaiscript::Module> make_bindings() { return nullptr; }
#define ADD_CLASS(Class) module->add(chaiscript::user_type<Class>(), #Class)
#define ADD_FUN(Class, Name) module->add(chaiscript::fun(&Class::Name), #Name)

/*
    Represents a scripted data type.
*/
struct ActorReflection
{
    struct Parameter
    {
        hash_t type;
        std::string name;
        std::tuple<float, float, float> range;
    };

    std::string name;
    std::vector<Parameter> parameters;
};

/*
    Represents an instantiated script object.
*/
struct Actor
{
    template <typename T> using SharedParameterMap = std::unordered_map<std::string, std::reference_wrapper<T>>;

    enum ActorTrait : uint8_t
    {
        NONE = 0,
        UPDATER = 1
    };

    Actor(InstanceHandle ih) : instance_handle(ih) {}
    inline void enable(bool value) { enabled_ = value; }
    inline bool is_enabled() const { return enabled_; }
    inline bool has_trait(ActorTrait trait) const { return (traits & trait); }
    inline void set_trait(ActorTrait trait) { traits |= trait; }

    template <typename T> inline void add_parameter(const std::string&, std::reference_wrapper<T>) {}
    template <typename T> inline std::reference_wrapper<T> get_parameter(const std::string&) { return {}; }
    template <typename T> inline T* get_parameter_ptr(const std::string&) { return nullptr; }
    template <typename T> inline bool has_parameter(const std::string&) const { return false; }

    void update_parameters(const Actor& other);

public:
    InstanceHandle instance_handle = 0;
    hash_t actor_type = 0;
    uint8_t traits = 0;
    std::function<void(float)> update;

private:
    bool enabled_ = true;
    SharedParameterMap<float> floats_;
};

template <> inline void Actor::add_parameter<float>(const std::string& name, std::reference_wrapper<float> tref)
{
    floats_.insert({name, tref});
}
template <> inline std::reference_wrapper<float> Actor::get_parameter<float>(const std::string& name)
{
    return floats_.at(name);
}
template <> inline float* Actor::get_parameter_ptr<float>(const std::string& name) { return &(floats_.at(name).get()); }
template <> inline bool Actor::has_parameter<float>(const std::string& name) const
{
    return floats_.find(name) != floats_.end();
}

/*
    Encapsulate the creation of ChaiScript objects. This allows to
    reduce build times.
    http://discourse.chaiscript.com/t/slow-build-times/94/3
*/
struct ChaiContext
{
    using VM_ptr = std::shared_ptr<chaiscript::ChaiScript>;
    using ActorVisitor = std::function<void(Actor&)>;
    VM_ptr vm;

    ChaiContext();
    ~ChaiContext() = default;

    inline VM_ptr operator->() { return vm; }
    inline auto& get_actor(ActorHandle idx) { return actors_.at(idx); }
    inline const auto& get_reflection(hash_t actor_type) const { return reflections_.at(actor_type); }
    inline void traverse_actors(ActorVisitor visit)
    {
        for(auto&& [hnd, actor] : actors_)
            if(actor.is_enabled())
                visit(actor);
    }

    void init(VMHandle handle);
    void add_bindings(std::shared_ptr<chaiscript::Module> module);
    hash_t use(const WPath& script_path);
    void eval(const std::string& command);
    ActorHandle instantiate(hash_t actor_type, EntityID e);
    void remove_actor(ActorHandle idx);

    void setup_component(ComponentScript& cscript, EntityID e);
    void update_parameters(const ChaiContext& other);

#ifdef W_DEBUG
    void dbg_dump_state(const std::string& outfile);
#endif

    struct Storage;

private:
    hash_t reflect(const WPath& script_path);

private:
    std::shared_ptr<Storage> storage_ = nullptr;
    std::map<ActorHandle, Actor> actors_;
    std::map<hash_t, ActorReflection> reflections_;
    std::map<hash_t, hash_t> used_files_;
    VMHandle handle_;
};

} // namespace script
} // namespace erwin