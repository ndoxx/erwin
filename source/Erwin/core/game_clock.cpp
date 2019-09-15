#include "core/game_clock.h"

namespace erwin
{

float GameClock::MAX_FRAME_SPEED_ = 5.0f;
float GameClock::SPEED_INCREMENT_ = 0.1f;

GameClock::GameClock():
frame_speed_(1.0f),
dt_(0.0f),
next_frame_required_(false),
pause_(false)
{

}

} // namespace erwin
