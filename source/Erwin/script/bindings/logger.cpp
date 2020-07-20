#include <chaiscript/chaiscript.hpp>
#include <string>

#include "debug/logger.h"
#include "script/bindings/logger.h"

namespace erwin
{
namespace script
{
std::shared_ptr<chaiscript::Module> make_logger_bindings()
{
	using namespace chaiscript;
    auto module = std::make_shared<chaiscript::Module>();

	module->add(fun([](const std::string& msg){ DLOG("script",1) << msg << std::endl; }), "DLOG");
	module->add(fun([](const std::string& msg){ DLOGN("script") << msg << std::endl; }), "DLOGN");
	module->add(fun([](const std::string& msg){ DLOGW("script") << msg << std::endl; }), "DLOGW");
	module->add(fun([](const std::string& msg){ DLOGE("script") << msg << std::endl; }), "DLOGE");
	module->add(fun([](){ DLOG("script",3) << WCC('d') << "BANG!" << std::endl; }), "BANG");

	return module;
}

} // namespace script
} // namespace erwin