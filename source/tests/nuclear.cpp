#include <array>
#include <atomic>
#include <bitset>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <queue>
#include <random>
#include <sstream>
#include <type_traits>
#include <vector>

#include "ctti/type_id.hpp"
#include "debug/logger.h"
#include "debug/logger_sink.h"
#include "debug/logger_thread.h"
#include "glm/glm.hpp"
#include "glm/gtx/string_cast.hpp"
#include "memory/memory.hpp"

#include "filesystem/wesh_file.h"

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

int main(int argc, char** argv)
{
    init_logger();

    auto descriptor = wesh::read("/home/ndx/erwin_workspace/test_scene/assets/mesh/cube.wesh");

    int cnt = 0;
    for(float ff: descriptor.vertex_data)
    {
        DLOGR("asset") << ff << " ";
        if(++cnt == 11)
        {
            cnt = 0;
            DLOGR("asset") << std::endl;
        }
    }

    for(uint32_t idx: descriptor.index_data)
    {
        DLOGR("asset") << idx << " ";
    }
    DLOGR("asset") << std::endl;

    return 0;
}