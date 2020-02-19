#include <iostream>
#include <iomanip>
#include <vector>
#include <functional>
#include <random>
#include <type_traits>
#include <cstdlib>
#include <cstring>
#include <bitset>
#include <atomic>

#include "glm/glm.hpp"
#include "memory/memory.hpp"
#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "debug/logger_sink.h"

#include "core/config.h"
#include "filesystem/filesystem.h"

using namespace erwin;

void init_logger()
{
    WLOGGER(create_channel("memory", 3));
    WLOGGER(create_channel("thread", 3));
	WLOGGER(create_channel("nuclear", 3));
	WLOGGER(create_channel("entity", 3));
	WLOGGER(create_channel("config", 3));
	WLOGGER(attach_all("console_sink", std::make_unique<dbg::ConsoleSink>()));
    WLOGGER(set_single_threaded(true));
    WLOGGER(set_backtrace_on_error(false));
    WLOGGER(spawn());
    WLOGGER(sync());

    DLOGN("nuclear") << "Nuclear test" << std::endl;
}

int main(int argc, char** argv)
{
	init_logger();
	filesystem::init();
    cfg::load(filesystem::get_root_dir() / "bin/test/test_config.xml");

    DLOG("nuclear",1) << "an_int: " << cfg::get<int>("test_config.foo.an_int"_h, 0) << std::endl;
    DLOG("nuclear",1) << "a_bool: " << cfg::get<bool>("test_config.baz.a_bool"_h, false) << std::endl;
    DLOG("nuclear",1) << "a_size: " << cfg::get<size_t>("test_config.foo.a_size"_h, 0) << std::endl;

    cfg::set<int>("test_config.foo.an_int"_h, 12);
    cfg::set<bool>("test_config.baz.a_bool"_h, true);
    cfg::set<size_t>("test_config.foo.a_size"_h, 22_MB);
    DLOG("nuclear",1) << "an_int: " << cfg::get<int>("test_config.foo.an_int"_h, 0) << std::endl;
    DLOG("nuclear",1) << "a_bool: " << cfg::get<bool>("test_config.baz.a_bool"_h, false) << std::endl;

    cfg::save(filesystem::get_root_dir() / "bin/test/test_config.xml");


	return 0;
}
