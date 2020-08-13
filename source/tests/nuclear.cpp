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

#include "filesystem/wpath.h"

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
    KLOGGER(create_channel("memory", 3));
    KLOGGER(create_channel("thread", 3));
    KLOGGER(create_channel("nuclear", 3));
    KLOGGER(create_channel("entity", 3));
    KLOGGER(create_channel("config", 3));
    KLOGGER(create_channel("render", 3));
    KLOGGER(create_channel("asset", 3));
    KLOGGER(attach_all("console_sink", std::make_unique<klog::ConsoleSink>()));
    KLOGGER(set_single_threaded(true));
    KLOGGER(set_backtrace_on_error(false));
    KLOGGER(spawn());
    KLOGGER(sync());

    DLOGN("nuclear") << "Nuclear test" << std::endl;
}

std::string to_str(uint32_t handle, const SecureSparsePool<uint32_t, 64, 16>& pool)
{
    std::stringstream ss;
    ss << std::bitset<32>(handle) << " [" << std::to_string(handle & pool.k_handle_mask) << '/'
       << std::to_string((handle & pool.k_guard_mask) >> pool.k_guard_shift) << ']';
    return ss.str();
}

int main(int argc, char** argv)
{
    init_logger();

    SecureSparsePool<uint32_t, 64, 16> handle_pool;

    DLOG("nuclear", 1) << std::bitset<32>(handle_pool.k_guard_mask) << std::endl;
    DLOG("nuclear", 1) << std::bitset<32>(handle_pool.k_handle_mask) << std::endl;

    DLOGN("nuclear") << "Testing" << std::endl;
    for(size_t ii = 0; ii < 10; ++ii)
    {
        handle_pool.acquire();
    }

    BANG();
    auto h10 = handle_pool.acquire();
    auto h11 = handle_pool.acquire();
    auto h12 = handle_pool.acquire();

    DLOG("nuclear", 1) << to_str(h10, handle_pool) << std::endl;
    DLOG("nuclear", 1) << to_str(h11, handle_pool) << std::endl;
    DLOG("nuclear", 1) << to_str(h12, handle_pool) << std::endl;

    BANG();

    DLOG("nuclear", 1) << std::boolalpha << handle_pool.is_valid(h10) << std::endl;
    DLOG("nuclear", 1) << std::boolalpha << handle_pool.is_valid(h11) << std::endl;
    DLOG("nuclear", 1) << std::boolalpha << handle_pool.is_valid(h12) << std::endl;

    handle_pool.release(h10);
    // handle_pool.release(h11);
    // handle_pool.release(h12);

    DLOG("nuclear", 1) << std::boolalpha << handle_pool.is_valid(h10) << std::endl;
    DLOG("nuclear", 1) << std::boolalpha << handle_pool.is_valid(h11) << std::endl;
    DLOG("nuclear", 1) << std::boolalpha << handle_pool.is_valid(h12) << std::endl;

    BANG();
    auto h_a = handle_pool.acquire();
    auto h_b = handle_pool.acquire();
    auto h_c = handle_pool.acquire();

    DLOG("nuclear", 1) << to_str(h_a, handle_pool) << std::endl;
    DLOG("nuclear", 1) << to_str(h_b, handle_pool) << std::endl;
    DLOG("nuclear", 1) << to_str(h_c, handle_pool) << std::endl;

    return 0;
}