#pragma once

#include <chrono>

#include "core/core.h"

namespace erwin
{

// Interface for a GPU-side query timer
class QueryTimer
{
public:
    virtual ~QueryTimer() = default;

    // Start query timer
    virtual void start(bool sync=false) = 0;
    // Stop timer and get elapsed GPU time
    virtual std::chrono::nanoseconds stop() = 0;

    // Factory method for the creation of a graphics API specific timer
    static WScope<QueryTimer> create();
};

} // namespace erwin
