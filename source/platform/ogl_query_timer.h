#pragma once

#include <cstdint>

#include "render/query_timer.h"

namespace erwin
{

class OGLQueryTimer: public QueryTimer
{
public:
	OGLQueryTimer();
	~OGLQueryTimer() = default;

    // Start query timer
    virtual void start() override;
    // Stop timer and get elapsed GPU time
    virtual std::chrono::nanoseconds stop() override;

private:
    uint32_t query_ID_[2]; // the array to store the two sets of queries.
    uint32_t query_back_buffer_;
    uint32_t query_front_buffer_;
    uint32_t timer_;
};


} // namespace erwin