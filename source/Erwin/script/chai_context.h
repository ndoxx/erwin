#pragma once

#include "entity/reflection.h"
#include "filesystem/wpath.h"
#include <functional>
#include <memory>
#include <vector>

namespace chaiscript
{
class ChaiScript;
class Module;
} // namespace chaiscript

namespace erwin
{
namespace script
{

static constexpr size_t k_max_actors = 128;
using ActorIndex = size_t;
using InstanceHandle = size_t;

template <typename ExposedT> std::shared_ptr<chaiscript::Module> make_bindings() { return nullptr; }
#define ADD_CLASS(Class) module->add(chaiscript::user_type<Class>(), #Class)
#define ADD_FUN(Class, Name) module->add(chaiscript::fun(&Class::Name), #Name)

/*
    Represents an instantiated script object.
*/
struct Actor
{
    enum ActorTrait : uint8_t
    {
        NONE = 0,
        UPDATER = 1
    };

    InstanceHandle instance_handle = 0;
    uint8_t traits = 0;

    std::function<void(float)> update;

    inline bool has_trait(ActorTrait trait) const { return (traits & trait); }
    inline void set_trait(ActorTrait trait) { traits |= trait; }
};

/*
    Encapsulate the creation of ChaiScript objects. This allows to
    reduce build times.
    http://discourse.chaiscript.com/t/slow-build-times/94/3
*/
struct ChaiContext
{
    using VM_ptr = std::shared_ptr<chaiscript::ChaiScript>;
    VM_ptr vm;

    ChaiContext() = default;
    ~ChaiContext();

    inline VM_ptr operator->() { return vm; }

    void init();
    void add_bindings(std::shared_ptr<chaiscript::Module> module);
    void use(const WPath& script_path);
    ActorIndex instantiate(const std::string& entry_point, EntityID e);
    inline Actor& get_actor(ActorIndex idx) { return actors_.at(idx); }

private:
    std::vector<Actor> actors_;
};

} // namespace script
} // namespace erwin