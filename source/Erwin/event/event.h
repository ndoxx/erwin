#pragma once

#include <string_view>

namespace erwin
{

#define EVENT_DECLARATIONS(EVENT_NAME) \
    static constexpr std::string_view NAME = #EVENT_NAME; \
    inline const std::string_view get_name() const { return NAME; }

} // namespace erwin