#include "entity/hierarchy.h"

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

void depth_first_recurse(size_t depth, EntityID node, entt::registry& registry, NodeVisitor visit)
{
    const auto& hier = registry.get<HierarchyComponent>(node);
    if(visit(node, hier, depth))
        return;

    auto curr = hier.first_child;
    while(curr != entt::null)
    {
        depth_first_recurse(depth + 1, curr, registry, visit);
        curr = registry.get<HierarchyComponent>(curr).next_sibling;
    }
}

void depth_first(EntityID node, entt::registry& registry, NodeVisitor visit)
{
    depth_first_recurse(0, node, registry, visit);
}

bool subtree_contains(EntityID root, EntityID node, entt::registry& registry)
{
	bool found = false;
    depth_first_recurse(0, root, registry, [&found, node](EntityID ent, const auto&, size_t)
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