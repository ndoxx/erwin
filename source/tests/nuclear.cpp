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

static inline std::string size_to_string(size_t size)
{
	static std::string sizes[] = {"_B", "_kB", "_MB", "_GB"};

	int ii = 0;
	while(size%1024 == 0 && ii < 4)
	{
	    size /= 1024;
	    ++ii;
	}

	return std::to_string(size) + sizes[ii];
}

int main(int argc, char** argv)
{
	init_logger();

	std::cout << size_to_string(1) << std::endl;
	std::cout << size_to_string(128) << std::endl;
	std::cout << size_to_string(1024) << std::endl;
	std::cout << size_to_string(1024*2) << std::endl;
	std::cout << size_to_string(1024*2+1) << std::endl;
	std::cout << size_to_string(1024*1024*458) << std::endl;
	std::cout << size_to_string(1024*1024*458+5) << std::endl;

	return 0;
}
