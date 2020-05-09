#pragma once

#include <string_view>
#include <cstdint>
#include "ctti/type_id.hpp"

namespace erwin
{

using EventID = uint64_t;

#define EVENT_DECLARATIONS(EVENT_NAME) \
    static constexpr std::string_view NAME = #EVENT_NAME; \
    static constexpr EventID ID = ctti::type_id< EVENT_NAME >().hash()

} // namespace erwin