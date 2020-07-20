#include "script/script_engine.h"
#include <chaiscript/chaiscript.hpp>

namespace erwin
{
namespace script
{

void ChaiContext::init() { vm = std::make_shared<chaiscript::ChaiScript>(); }

void ChaiContext::add_bindings(std::shared_ptr<chaiscript::Module> module) { vm->add(module); }

} // namespace script
} // namespace erwin
