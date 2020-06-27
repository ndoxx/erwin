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

inline std::string to_string(EntityID entity)
{
    return (entity != k_invalid_entity_id) ? std::to_string(size_t(entity)) : "NULL";
}

std::ostream& operator<<(std::ostream& stream, const ComponentHierarchy& rhs)
{
    stream << "nc: " << rhs.children << " pa: " << to_string(rhs.parent) << " pr: " << to_string(rhs.previous_sibling)
           << " ne: " << to_string(rhs.next_sibling) << " fc: " << to_string(rhs.first_child);
    return stream;
}

struct NameComponent
{
    NameComponent(const std::string& name_) : name(name_) {}
    std::string name;
};

using NodeVisitor = std::function<bool(EntityID, const ComponentHierarchy&, size_t)>;
using Snapshot = std::pair<EntityID, size_t>; // Store entity along its depth

void iterative_depth_first(EntityID node, entt::registry& registry, NodeVisitor visit)
{
    std::stack<Snapshot> candidates;

    // Push the current source node.
    candidates.push({node, 0});

    while (!candidates.empty())
    {
        auto [ent, depth] = candidates.top();
        candidates.pop();

        const auto& hier = registry.get<ComponentHierarchy>(ent);
        if(visit(ent, hier, depth))
            break;

        // Push all children to the stack
        auto child = hier.first_child;

        while(child != entt::null)
        {
            candidates.push({child, depth+1});
            child = registry.get<ComponentHierarchy>(child).next_sibling;
        }
    }
}

void recursive_depth_first(size_t depth, EntityID node, entt::registry& registry, NodeVisitor visit)
{
    const auto& hier = registry.get<ComponentHierarchy>(node);
    if(visit(node, hier, depth))
        return;

    auto curr = hier.first_child;
    while(curr != entt::null)
    {
        recursive_depth_first(depth + 1, curr, registry, visit);
        curr = registry.get<ComponentHierarchy>(curr).next_sibling;
    }
}

void print_hierarchy(EntityID node, entt::registry& registry)
{
    entity::depth_first(node, registry, [&registry](EntityID ent, const auto& hier, size_t depth) {
        std::string indent(4 * depth, ' ');
        DLOGN("nuclear") << indent << "[" << size_t(ent) << "] @" << depth << std::endl;
        DLOG("nuclear", 1) << indent << hier << std::endl;
        return false;
    });
}

struct GenParams
{
    size_t seed = 42;
    size_t max_nodes = 20;
    size_t max_children = 3;
    size_t n_iter = 2 * max_nodes * max_children;
    float prob_create_child = 0.5f;
};

struct Dummy
{
    int value = 0;
};

void create_random_hierarchy(const GenParams& gp, EntityID root, entt::registry& registry)
{
    std::mt19937 gen(gp.seed);
    std::uniform_real_distribution<float> dis(0.f, 1.f);

    registry.emplace<Dummy>(root);
    registry.emplace<ComponentHierarchy>(root);
    std::vector<EntityID> nodes;
    nodes.push_back(root);

    size_t ii = 0;
    while(ii < gp.n_iter && nodes.size() < gp.max_nodes)
    {
        // Select random node
        auto rnode = *random_select(nodes.begin(), nodes.end(), gen);
        const auto& r_hier = registry.get<ComponentHierarchy>(rnode);

        // If test passed, create child
        if(r_hier.children < gp.max_children && dis(gen) > gp.prob_create_child)
        {
            auto child = registry.create();
            registry.emplace<Dummy>(child);
            nodes.push_back(child);

            entity::attach(rnode, child, registry);
        }

        ++ii;
    }
}

int main(int argc, char** argv)
{
    init_logger();

    entt::registry registry;

    // GenParams gen_params{42, 20, 5, 100, 0.5f};
    GenParams gen_params{42, 1000, 8, 50000, 0.5f};

    size_t seed = 42;
    auto root = registry.create();
    create_random_hierarchy(gen_params, root, registry);
    entity::sort_hierarchy(registry);

#if 0
    print_hierarchy(root, registry);
#endif

#if 1
    size_t n_iter = 1000;

    // Ground duration
    unsigned long long int ground_duration = 0ull;
    for(size_t ii=0; ii<n_iter; ++ii)
    {
        nanoClock clk;
        ground_duration += std::chrono::duration_cast<std::chrono::nanoseconds>(clk.get_elapsed_time()).count();
    }
    long double ground_duration_d = static_cast<long double>(ground_duration) / static_cast<long double>(n_iter);

    DLOG("nuclear",1) << "Ground duration: " << ground_duration_d << "ns" << std::endl;
    DLOG("nuclear", 1) << "---------------------------" << std::endl;

    {
        DLOG("nuclear", 1) << "Recursive depth first traversal" << std::endl;

        unsigned long long int mean_duration = 0ull;
        for(size_t ii=0; ii<n_iter; ++ii)
        {
            nanoClock clk;

            {
                recursive_depth_first(0, root, registry, [&registry](EntityID ent, const ComponentHierarchy&, size_t)
                {
                    auto& dummy = registry.get<Dummy>(ent);
                    ++dummy.value;
                    return false;
                });
            }

            mean_duration += std::chrono::duration_cast<std::chrono::nanoseconds>(clk.get_elapsed_time()).count();
        }
        long double mean_duration_d = std::max(0.0L, static_cast<long double>(mean_duration) / static_cast<long double>(n_iter) - ground_duration_d);

        DLOG("nuclear",1) << "Mean duration: " << mean_duration_d << "ns" << std::endl;
        DLOG("nuclear", 1) << "---------------------------" << std::endl;
    }
    
    {
        DLOG("nuclear", 1) << "Iterative depth first traversal" << std::endl;

        unsigned long long int mean_duration = 0ull;
        for(size_t ii=0; ii<n_iter; ++ii)
        {
            nanoClock clk;

            {
                iterative_depth_first(root, registry, [&registry](EntityID ent, const auto&, size_t)
                {
                    auto& dummy = registry.get<Dummy>(ent);
                    ++dummy.value;
                    return false;
                });
            }

            mean_duration += std::chrono::duration_cast<std::chrono::nanoseconds>(clk.get_elapsed_time()).count();
        }
        long double mean_duration_d = std::max(0.0L, static_cast<long double>(mean_duration) / static_cast<long double>(n_iter) - ground_duration_d);

        DLOG("nuclear",1) << "Mean duration: " << mean_duration_d << "ns" << std::endl;
        DLOG("nuclear", 1) << "---------------------------" << std::endl;
    }
#endif

    return 0;
}