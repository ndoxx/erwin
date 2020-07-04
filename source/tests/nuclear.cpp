#include <array>
#include <atomic>
#include <bitset>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <numeric>
#include <queue>
#include <random>
#include <sstream>
#include <type_traits>
#include <vector>
#include <stack>
#include <filesystem>

#include "ctti/type_id.hpp"
#include "debug/logger.h"
#include "debug/logger_sink.h"
#include "debug/logger_thread.h"
#include "glm/glm.hpp"
#include "glm/gtx/string_cast.hpp"
#include "memory/memory.hpp"
#include "core/clock.hpp"
#include "utils/sparse_set.hpp"
#include "utils/random_operations.hpp"

#include "entity/component/hierarchy.h"
#include "entity/component/transform.h"
#include "entt/entt.hpp"

using namespace erwin;

void init_logger()
{
    WLOGGER(create_channel("memory", 3));
    WLOGGER(create_channel("thread", 3));
    WLOGGER(create_channel("nuclear", 3));
    WLOGGER(create_channel("entity", 3));
    WLOGGER(create_channel("config", 3));
    WLOGGER(create_channel("render", 3));
    WLOGGER(create_channel("asset", 3));
    WLOGGER(attach_all("console_sink", std::make_unique<dbg::ConsoleSink>()));
    WLOGGER(set_single_threaded(true));
    WLOGGER(set_backtrace_on_error(false));
    WLOGGER(spawn());
    WLOGGER(sync());

    DLOGN("nuclear") << "Nuclear test" << std::endl;
}

fs::path operator "" _p(const char* fullpath, unsigned long)
{
    return fs::path(fullpath);
}

FilePath operator "" _fp(const char* fullpath, unsigned long)
{
    return FilePath(fullpath);
}

int main(int argc, char** argv)
{
    init_logger();

    return 0;
}