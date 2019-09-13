#pragma once

#include <cstdint>

#include "render/query_timer.h"

namespace erwin
{

class OGLQueryTimer: public QueryTimer
{
public:
	OGLQueryTimer();
	~OGLQueryTimer();

    // Start query timer
    virtual void start() override;
    // Stop timer and get elapsed GPU time
    virtual std::chrono::nanoseconds stop() override;

private:
    // Helper func to swap query buffers
    void swap_query_buffers();

private:
    // the array to store the two sets of queries.
    unsigned int* query_ID_;
    unsigned int query_back_buffer_;
    unsigned int query_front_buffer_;
    uint32_t timer_;
};


} // namespace erwin