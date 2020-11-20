#pragma once

#include <kibble/time/clock.h>

namespace erwin
{

#ifdef W_DEBUG
namespace dbg
{
static kb::nanoClock debug_clock;
}

inline void TIC_()
{
    dbg::debug_clock.restart();
}

inline float TOC_()
{
    auto period = dbg::debug_clock.get_elapsed_time();
    return std::chrono::duration_cast<std::chrono::duration<float>>(period).count();
}
#endif

}

