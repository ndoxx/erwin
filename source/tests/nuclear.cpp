#include <array>
#include <atomic>
#include <bitset>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
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

#include "entity/component_transform.h"
#include "entity/hierarchy.h"
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

inline std::string to_string(EntityID entity)
{
    return (entity != k_invalid_entity_id) ? std::to_string(size_t(entity)) : "NULL";
}

std::ostream& operator<<(std::ostream& stream, const HierarchyComponent& rhs)
{
    stream << "nc: " << rhs.children
           << " pa: " << to_string(rhs.parent)
           << " pr: " << to_string(rhs.previous_sibling)
           << " ne: " << to_string(rhs.next_sibling)
           << " fc: " << to_string(rhs.first_child);
    return stream;
}

struct NameComponent
{
    NameComponent(const std::string& name_): name(name_) {}
    std::string name;
};

void print_hierarchy(EntityID node, size_t depth, entt::registry& registry)
{
    const auto& hier = registry.get<HierarchyComponent>(node);
    const auto& cname = registry.get<NameComponent>(node);
    std::string indent(4*depth, ' ');
    DLOGN("nuclear") << indent << cname.name << " [" << size_t(node) << "]" << std::endl;
    DLOG("nuclear", 1) << indent << hier << std::endl << std::endl;

    auto curr = hier.first_child;

    while(curr != entt::null)
    {
        print_hierarchy(curr, depth + 1, registry);
        curr = registry.get<HierarchyComponent>(curr).next_sibling;
    }
}

int main(int argc, char** argv)
{
    init_logger();

    entt::registry registry;

    auto A = registry.create();
    auto B = registry.create();
    auto C = registry.create();
    auto D = registry.create();
    auto E = registry.create();
    auto F = registry.create();

    registry.emplace<NameComponent>(A, "A");
    registry.emplace<NameComponent>(B, "B");
    registry.emplace<NameComponent>(C, "C");
    registry.emplace<NameComponent>(D, "D");
    registry.emplace<NameComponent>(E, "E");
    registry.emplace<NameComponent>(F, "F");

    entity::attach(A, B, registry);
    entity::attach(A, C, registry);
    entity::attach(A, D, registry);

    entity::attach(D, E, registry);
    entity::attach(D, F, registry);

    entity::sort_hierarchy(registry);
    // registry.view<HierarchyComponent, NameComponent>().each([](auto e, const auto& hier, const auto& cname) {
    //     DLOGN("nuclear") << cname.name << std::endl;
    // });

    print_hierarchy(A, 0, registry);

    DLOG("nuclear", 1) << "---------------------------" << std::endl;

    entity::attach(C, D, registry);
    print_hierarchy(A, 0, registry);


    return 0;
}