#pragma once

#include <memory>

namespace chaiscript
{
class ChaiScript;
class Module;
} // namespace chaiscript

namespace erwin
{
namespace script
{

template <typename ExposedT> std::shared_ptr<chaiscript::Module> make_bindings() { return nullptr; }
#define ADD_CLASS(Class) module->add(chaiscript::user_type<Class>(), #Class)
#define ADD_FUN(Class, Name) module->add(chaiscript::fun(&Class::Name), #Name)

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
    ~ChaiContext() = default;

    inline VM_ptr operator->() { return vm; }

    void init();
    void add_bindings(std::shared_ptr<chaiscript::Module> module);
};

} // namespace script
} // namespace erwin