#pragma once

#include <cstdint>

namespace erwin
{
namespace script
{

static constexpr std::size_t k_max_actors = 128;
static constexpr std::size_t k_max_script_vms = 8;
using ActorHandle = std::size_t;
using InstanceHandle = std::size_t;
using VMHandle = std::size_t;

} // namespace script
} // namespace erwin