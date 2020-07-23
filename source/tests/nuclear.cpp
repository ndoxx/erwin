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
#include "debug/logger.h"
#include "debug/logger_sink.h"
#include "debug/logger_thread.h"
#include "glm/glm.hpp"
#include "glm/gtx/string_cast.hpp"
#include "memory/memory.hpp"
#include "utils/random_operations.hpp"
#include "utils/sparse_set.hpp"

#include "entity/component/hierarchy.h"
#include "entity/component/transform.h"
#include "entity/component/camera.h"
#include "entity/component/bounding_box.h"
#include "entity/component/mesh.h"
#include "entity/component/PBR_material.h"
#include "entity/component/dirlight_material.h"
#include "entity/component/light.h"
#include "entity/component/description.h"

#include "entt/entt.hpp"

#include "filesystem/wpath.h"

using namespace erwin;

// TMP
class Scene;
template <> void erwin::inspector_GUI<ComponentTransform3D>(ComponentTransform3D&, EntityID, Scene&) {}
template <> void erwin::inspector_GUI<ComponentCamera3D>(ComponentCamera3D&, EntityID, Scene&) {}
template <> void erwin::inspector_GUI<ComponentOBB>(ComponentOBB&, EntityID, Scene&) {}
template <> void erwin::inspector_GUI<ComponentMesh>(ComponentMesh&, EntityID, Scene&) {}
template <> void erwin::inspector_GUI<ComponentPBRMaterial>(ComponentPBRMaterial&, EntityID, Scene&) {}
template <> void erwin::inspector_GUI<ComponentDirectionalLightMaterial>(ComponentDirectionalLightMaterial&, EntityID, Scene&) {}
template <> void erwin::inspector_GUI<ComponentDirectionalLight>(ComponentDirectionalLight&, EntityID, Scene&) {}
template <> void erwin::inspector_GUI<ComponentScript>(ComponentScript&, EntityID, Scene&) {}

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

void show(const WPath& p)
{
    BANG();
    DLOG("nuclear",1) << p << std::endl;
    DLOG("nuclear",1) << p.absolute() << std::endl;
    DLOG("nuclear",1) << p.relative() << std::endl;
}

int main(int argc, char** argv)
{
    init_logger();

    WPath::set_root_directory("/home/ndx/dev/Erwin");
    WPath::set_resource_directory("/home/ndx/erwin_workspace/test_scene/assets");

    auto p = "res://textures/materials/dirtyWickerWeave.tom"_wp;
    show(p);

    auto p2 = "/home/ndx/erwin_workspace/test_scene/assets/textures/materials/dirtyWickerWeave.tom"_wp;
    show(p2);

    return 0;
}