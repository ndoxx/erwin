#pragma once

namespace erwin
{

// Interface for a GPU-side query timer
class QueryTimer
{
public:
    virtual ~QueryTimer() = default;

    // Start query timer
    virtual void start() = 0;
    // Stop timer and get elapsed GPU time in s
    virtual float stop() = 0;

    // Factory method for the creation of a graphics API specific timer
    static QueryTimer* create();
};

} // namespace erwin
