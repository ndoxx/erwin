#pragma once

#include "core/core.h"
#include "entity/reflection.h"
#include <array>
#include <functional>

namespace erwin
{

struct HierarchyComponent
{
    std::size_t children = 0;
    entt::entity parent{k_invalid_entity_id};
    entt::entity first_child{k_invalid_entity_id};
    entt::entity previous_sibling{k_invalid_entity_id};
    entt::entity next_sibling{k_invalid_entity_id};
};

namespace entity
{
// To be used with traversal algorithms. Return true to stop exploration in current branch.
using NodeVisitor = std::function<bool(EntityID, const HierarchyComponent&, size_t /*relative_depth*/)>;

// Attach a subtree whose root is 'child' as a child of 'parent'
void attach(EntityID parent, EntityID child, entt::registry& registry);
// Detach a subtree whose root is 'node' from its parent
void detach(EntityID node, entt::registry& registry);
// Sort HierarchyComponent pool such that parents are always visited before their children
void sort_hierarchy(entt::registry& registry);
// Traverse hierarchy using a depth first algorithm and visit each node
void depth_first(EntityID node, entt::registry& registry, NodeVisitor visit);
// Check if subtree of root 'root' contains the node 'node'
bool subtree_contains(EntityID root, EntityID node, entt::registry& registry); 
// Check if node 'node' is a direct child of node 'parent'
bool is_child(EntityID parent, EntityID node, entt::registry& registry);
// Check if two nodes are siblings
bool is_sibling(EntityID first, EntityID second, entt::registry& registry);

} // namespace entity
} // namespace erwin