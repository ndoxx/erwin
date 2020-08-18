#include <array>
#include <atomic>
#include <bitset>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <numeric>
#include <queue>
#include <random>
#include <sstream>
#include <stack>
#include <type_traits>
#include <vector>

#include "core/clock.hpp"
#include "ctti/type_id.hpp"
#include "glm/glm.hpp"
#include "glm/gtx/string_cast.hpp"
#include "kibble/logger/logger.h"
#include "kibble/logger/logger_sink.h"
#include "kibble/logger/logger_thread.h"
#include "memory/memory.hpp"
#include "utils/random_operations.hpp"
#include "utils/sparse_set.hpp"

#include "entity/component/PBR_material.h"
#include "entity/component/bounding_box.h"
#include "entity/component/camera.h"
#include "entity/component/description.h"
#include "entity/component/dirlight_material.h"
#include "entity/component/hierarchy.h"
#include "entity/component/light.h"
#include "entity/component/mesh.h"
#include "entity/component/script.h"
#include "entity/component/transform.h"

#include "entt/entt.hpp"

#include "debug/net_sink.h"

using namespace erwin;
using namespace kb;

// TMP
class Scene;
template <> void erwin::inspector_GUI<ComponentTransform3D>(ComponentTransform3D&, EntityID, Scene&) {}
template <> void erwin::inspector_GUI<ComponentCamera3D>(ComponentCamera3D&, EntityID, Scene&) {}
template <> void erwin::inspector_GUI<ComponentOBB>(ComponentOBB&, EntityID, Scene&) {}
template <> void erwin::inspector_GUI<ComponentMesh>(ComponentMesh&, EntityID, Scene&) {}
template <> void erwin::inspector_GUI<ComponentPBRMaterial>(ComponentPBRMaterial&, EntityID, Scene&) {}
template <>
void erwin::inspector_GUI<ComponentDirectionalLightMaterial>(ComponentDirectionalLightMaterial&, EntityID, Scene&)
{}
template <> void erwin::inspector_GUI<ComponentDirectionalLight>(ComponentDirectionalLight&, EntityID, Scene&) {}
template <> void erwin::inspector_GUI<ComponentScript>(ComponentScript&, EntityID, Scene&) {}

void init_logger()
{
    KLOGGER_START();
    KLOGGER(create_channel("application", 3));
    KLOGGER(create_channel("memory", 3));
    KLOGGER(create_channel("thread", 3));
    KLOGGER(create_channel("nuclear", 3));
    KLOGGER(create_channel("entity", 3));
    KLOGGER(create_channel("config", 3));
    KLOGGER(create_channel("render", 3));
    KLOGGER(create_channel("asset", 3));
    KLOGGER(attach_all("console_sink", std::make_unique<klog::ConsoleSink>()));

    {
        auto net_sink = std::make_unique<NetSink>();
        if(net_sink->connect("localhost", 31337))
        {
            std::cout << "CONNECTED" << std::endl;
            KLOGGER(attach_all("net_sink", std::move(net_sink)));
        }
    }

    KLOGGER(set_single_threaded(true));
    KLOGGER(set_backtrace_on_error(false));
    KLOGGER(spawn());
    KLOGGER(sync());

    DLOGN("nuclear") << "Nuclear test" << std::endl;
}

int main(int argc, char** argv)
{
    init_logger();

    DLOG("memory",0) << "Plip plop" << std::endl;
    DLOG("thread",0) << "Plip plop" << std::endl;
    DLOG("entity",0) << "Plip plop" << std::endl;
    DLOG("config",0) << "Plip plop" << std::endl;
    DLOG("render",0) << "Plip plop" << std::endl;
    DLOG("asset",0)  << "Plip plop" << std::endl;
    DLOGI << "Item 1" << std::endl;
    DLOGI << "Item 2" << std::endl;
    DLOGI << "Item 3" << std::endl;

    DLOGN("application") << "Notification" << std::endl;
    DLOGW("application") << "Warning" << std::endl;
    DLOGE("application") << "Error" << std::endl;
    DLOGF("application") << "Fatal" << std::endl;
    DLOGG("application") << "Good" << std::endl;
    DLOGB("application") << "Bad" << std::endl;
    BANG();

    return 0;
}