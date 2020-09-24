#pragma once

#include <chrono>
#include <future>

namespace erwin
{

// Helper function to determine if a future has its value set
template <typename T> inline bool is_ready(std::future<T> const& fut)
{
    return fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

} // namespace erwin