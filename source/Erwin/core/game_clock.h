#pragma once

#include <chrono>

namespace erwin
{

class GameClock
{
public:
    inline bool is_next_frame_required() const { return next_frame_required_; }
    inline bool is_paused() const { return pause_; }
    inline float get_frame_speed() const { return next_frame_required_?1.0f:frame_speed_; }

    inline void toggle_pause() { pause_ = ! pause_; }
    inline void set_frame_speed(float value) { frame_speed_ = value; }
    inline void require_next_frame() { if(frame_speed_ == 0.0f) next_frame_required_ = true; }

    inline void frame_speed_up()
    {
        frame_speed_ += SPEED_INCREMENT_;
        if(frame_speed_ > MAX_FRAME_SPEED_)
            frame_speed_ = MAX_FRAME_SPEED_;
    }
    inline void frame_slow_down()
    {
        frame_speed_ -= SPEED_INCREMENT_;
        if(frame_speed_ < 0.0f)
            frame_speed_ = 0.0f;
    }

    inline void update(std::chrono::nanoseconds dt) { dt_ = float(dt.count())/float(1.0e9); }
    inline void release_flags() { next_frame_required_ = false; }
    inline float get_scaled_frame_duration() const { return get_frame_speed()*dt_; }
    inline float get_frame_duration() const { return dt_; }

private:
    float frame_speed_ = 1.f;
    float dt_ = 0.f;
    bool next_frame_required_ = false;
    bool pause_ = false;

    static constexpr float MAX_FRAME_SPEED_ = 5.f;
    static constexpr float SPEED_INCREMENT_ = 0.1f;
};

} // namespace erwin