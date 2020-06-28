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

struct CA
{
    CA(int A): a(A) {}
    int a = 0;
};

struct CB
{
    CB() = default;
    CB(int B): b(B) {}
    inline void init(int B) { b = B; }
    int b = 0;
};

static void update_b(entt::registry& reg, EntityID e)
{
    const auto& ca = reg.get<CA>(e);
    auto& cb = reg.get<CB>(e);
    cb.init(2 * ca.a);
}

int main(int argc, char** argv)
{
    init_logger();

    entt::registry registry;

    registry.on_update<CA>().connect<&update_b>();

    auto e1 = registry.create();
    // auto e2 = registry.create();

    auto& ca1 = registry.emplace<CA>(e1, 10);
    auto& cb1 = registry.emplace<CB>(e1);

    /*registry.view<CA>().each([](auto e, auto& ca)
    {
        ++ca.a;
    });*/
    registry.replace<CA>(e1, 15);

    DLOG("nuclear",1) << ca1.a << " " << cb1.b << std::endl;

    return 0;
}