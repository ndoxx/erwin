#pragma once

#include "core/core.h"
#include "entity/reflection.h"
#include <array>
#include <functional>

namespace erwin
{

struct ComponentHierarchy
{
    std::size_t children = 0;
    entt::entity parent{k_invalid_entity_id};
    entt::entity first_child{k_invalid_entity_id};
    entt::entity previous_sibling{k_invalid_entity_id};
    entt::entity next_sibling{k_invalid_entity_id};
};

} // namespace erwin