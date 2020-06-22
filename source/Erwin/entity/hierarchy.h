#pragma once

#include <array>
#include "core/core.h"
#include "entity/reflection.h"

namespace erwin
{


struct HierarchyComponent
{
	std::size_t children = 0;
    entt::entity parent {k_invalid_entity_id};
	entt::entity first_child {k_invalid_entity_id};
    entt::entity previous_sibling {k_invalid_entity_id};
    entt::entity next_sibling {k_invalid_entity_id};
};

namespace entity
{

void attach(EntityID parent, EntityID child, entt::registry& registry);
void detach(EntityID node, entt::registry& registry);
void sort_hierarchy(entt::registry& registry);

} // namespace entity
} // namespace erwin