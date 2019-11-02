#include <iostream>
#include <iomanip>
#include <vector>
#include <functional>
#include <random>
#include <type_traits>
#include <cstdlib>
#include <cstring>
#include <bitset>

#include "debug/logger.h"
#include "debug/logger_thread.h"
#include "debug/logger_sink.h"
#include "filesystem/filesystem.h"
#include "core/config.h"

using namespace erwin;

int main(int argc, char** argv)
{
    // Initialize file system
    filesystem::init();
    // Initialize config
    cfg::init(filesystem::get_config_dir() / "erwin.xml");

    std::cout << cfg::get<uint32_t>("erwin.display.width"_h, 1920) << std::endl;
    std::cout << cfg::get<size_t>("erwin.memory.test"_h, 1024*1024) << std::endl;

	return 0;
}
