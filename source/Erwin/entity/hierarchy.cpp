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
    else
    {
        // If previous sibling is null, it means that this child is the first child of its parent
        // then, parent needs to be updated too
        auto& parent_hierarchy = registry.get<HierarchyComponent>(node_hierarchy.parent);
        parent_hierarchy.first_child = node_hierarchy.next_sibling;
    }

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

} // namespace entity
} // namespace erwin