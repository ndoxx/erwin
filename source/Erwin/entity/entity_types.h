#pragma once

#include <cstdint>
#include "EASTL/numeric_limits.h"

namespace erwin
{

typedef uint64_t ComponentID;
typedef uint64_t EntityID;
static constexpr std::size_t k_invalid_pool_index = eastl::numeric_limits<std::size_t>::max();
static constexpr uint64_t k_invalid_entity_id = eastl::numeric_limits<EntityID>::max();

} // namespace erwin