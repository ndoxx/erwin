#include "entity/component/hierarchy.h"
#include "utils/sparse_set.hpp"

#include <stack>
#include <queue>

namespace erwin
{
namespace entity
{
void attach(EntityID parent, EntityID child, entt::registry& registry)
{
    // Can't seem to use entt::get_or_emplace() here as creating a new component
    // can invalidate previously acquired reference, resulting in a segfault later on
    if(!registry.has<HierarchyComponent>(parent))
        registry.emplace<HierarchyComponent>(parent);
    if(!registry.has<HierarchyComponent>(child))
        registry.emplace<HierarchyComponent>(child);

    auto& parent_hierarchy = registry.get<HierarchyComponent>(parent);
    auto& child_hierarchy = registry.get<HierarchyComponent>(child);

    // Make sure we don't try to attach a node to itself or one of its children
	W_ASSERT_FMT(!subtree_contains(child, parent, registry), "Child node [%zu] cannot be the ancestor of its parent [%zu].", size_t(child), size_t(parent));

    // If child was already assigned a parent, detach it from tree
    if(child_hierarchy.parent != k_invalid_entity_id)
        detach(child, registry);

    child_hierarchy.parent = parent;
    child_hierarchy.next_sibling = parent_hierarchy.first_child;

    if(parent_hierarchy.first_child != k_invalid_entity_id)
    {
        auto& first_sibling = registry.get<HierarchyComponent>(parent_hierarchy.first_child);
        first_sibling.previous_sibling = child;
    }

    parent_hierarchy.first_child = child;
    ++parent_hierarchy.children;
}

void detach(EntityID node, entt::registry& registry)
{
    auto& node_hierarchy = registry.get<HierarchyComponent>(node);
    auto& parent_hierarchy = registry.get<HierarchyComponent>(node_hierarchy.parent);
    W_ASSERT(node_hierarchy.parent != k_invalid_entity_id, "Cannot detach orphan node.");

    // Stitch back siblings if any
    if(node_hierarchy.next_sibling != k_invalid_entity_id)
    {
        auto& next_sibling = registry.get<HierarchyComponent>(node_hierarchy.next_sibling);
        next_sibling.previous_sibling = node_hierarchy.previous_sibling;
    }
    if(node_hierarchy.previous_sibling != k_invalid_entity_id)
    {
        auto& previous_sibling = registry.get<HierarchyComponent>(node_hierarchy.previous_sibling);
        previous_sibling.next_sibling = node_hierarchy.next_sibling;
    }
    // If previous sibling is null, it means that this child is the first child of its parent
    // then, parent needs to be updated too
    else
        parent_hierarchy.first_child = node_hierarchy.next_sibling;

    --parent_hierarchy.children;
    node_hierarchy.parent = k_invalid_entity_id;
    node_hierarchy.previous_sibling = k_invalid_entity_id;
    node_hierarchy.next_sibling = k_invalid_entity_id;
}

void sort_hierarchy(entt::registry& registry)
{
    registry.sort<HierarchyComponent>([&registry](const entt::entity lhs, const entt::entity rhs) {
        const auto& clhs = registry.get<HierarchyComponent>(lhs);
        const auto& crhs = registry.get<HierarchyComponent>(rhs);
        return crhs.parent == lhs || clhs.next_sibling == rhs ||
               (!(clhs.parent == rhs || crhs.next_sibling == lhs) &&
                (clhs.parent < crhs.parent || (clhs.parent == crhs.parent && &clhs < &crhs)));
    });
}

using Snapshot = std::pair<EntityID, size_t>; // Store entity along its depth

void depth_first(EntityID node, entt::registry& registry, NodeVisitor visit)
{
    DynamicSparseSet<size_t> visited; // O(1) search, ideal for this job!
    std::stack<Snapshot> candidates;

    // Push the current source node.
    candidates.push({node, 0});

    while (!candidates.empty())
    {
        auto& [ent, depth] = candidates.top();
        candidates.pop();

        const auto& hier = registry.get<HierarchyComponent>(ent);

        // Stack may contain same node twice.
        if(!visited.has(size_t(ent)))
        {
            if(visit(ent, hier, depth))
                break;
            visited.insert(size_t(ent));
        }

        // Push all children to the stack
        // They must however be pushed in the reverse order they appear for
        // the first child to be visited first during the next iteration
        // std::vector was measured to be faster here than std::stack
        auto child = hier.first_child;
        std::vector<Snapshot> children;
        while(child != entt::null)
        {
            if(!visited.has(size_t(child)))
                children.push_back({child, depth+1});
            child = registry.get<HierarchyComponent>(child).next_sibling;
        }
        for(auto it=children.rbegin(); it!=children.rend(); ++it)
            candidates.push(*it);
    }
}

void breadth_first(EntityID node, entt::registry& registry, NodeVisitor visit)
{
    DynamicSparseSet<size_t> visited;
    std::queue<Snapshot> candidates;

    // Push the current source node.
    candidates.push({node, 0});

    while (!candidates.empty())
    {
        auto& [ent, depth] = candidates.front();
        candidates.pop();

        const auto& hier = registry.get<HierarchyComponent>(ent);

        // Queue may contain same node twice.
        if(!visited.has(size_t(ent)))
        {
            if(visit(ent, hier, depth))
                break;
            visited.insert(size_t(ent));
        }

        // Push all children to the queue
        auto child = hier.first_child;
        while(child != entt::null)
        {
            if(!visited.has(size_t(child)))
                candidates.push({child, depth+1});
            child = registry.get<HierarchyComponent>(child).next_sibling;
        }
    }
}

bool subtree_contains(EntityID root, EntityID node, entt::registry& registry)
{
	bool found = false;
    depth_first(root, registry, [&found, node](EntityID ent, const auto&, size_t)
    {
    	found |= (ent == node);
    	return found;
    });

    return found;
}

bool is_child(EntityID parent, EntityID node, entt::registry& registry)
{
    const auto& parent_hierarchy = registry.get<HierarchyComponent>(parent);

    auto curr = parent_hierarchy.first_child;
    while(curr != entt::null)
    {
    	if(node == curr)
    		return true;
        curr = registry.get<HierarchyComponent>(curr).next_sibling;
    }
    return false;
}

bool is_sibling(EntityID first, EntityID second, entt::registry& registry)
{
    const auto& hierarchy_1 = registry.get<HierarchyComponent>(first);
    const auto& hierarchy_2 = registry.get<HierarchyComponent>(second);
    if(hierarchy_1.parent == k_invalid_entity_id ||
       hierarchy_2.parent == k_invalid_entity_id ||
       hierarchy_1.parent != hierarchy_2.parent)
    	return false;

    const auto& parent_hierarchy = registry.get<HierarchyComponent>(hierarchy_1.parent);

    auto curr = parent_hierarchy.first_child;
    while(curr != entt::null)
    {
    	if(second == curr)
    		return true;
        curr = registry.get<HierarchyComponent>(curr).next_sibling;
    }
    return false;
}


} // namespace entity
} // namespace erwin