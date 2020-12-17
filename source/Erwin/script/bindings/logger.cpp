#include <chaiscript/chaiscript.hpp>
#include <string>

#include "script/bindings/logger.h"
#include <kibble/logger/logger.h>



namespace erwin
{
namespace script
{
std::shared_ptr<chaiscript::Module> make_logger_bindings()
{
    using namespace chaiscript;
    auto module = std::make_shared<chaiscript::Module>();

    module->add(fun([](const std::string& msg) { KLOG("script", 1) << "chai> " << msg << std::endl; }), "KLOG");
    module->add(fun([](const std::string& msg) { KLOGN("script") << "chai> " << msg << std::endl; }), "KLOGN");
    module->add(fun([](const std::string& msg) { KLOGW("script") << "chai> " << msg << std::endl; }), "KLOGW");
    module->add(fun([](const std::string& msg) { KLOGE("script") << "chai> " << msg << std::endl; }), "KLOGE");
    module->add(fun([]() { KLOG("script", 1) << kb::KS_DEFL_ << "chai> ----[BANG]----" << std::endl; }), "BANG");

    return module;
}

} // namespace script
} // namespace erwin