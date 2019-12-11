#pragma once

#include "core/game_clock.h"

namespace erwin
{

class ComponentSystem
{
public:
	virtual bool init() = 0;
	virtual void update(const GameClock& clock) = 0;

private:
};


} // namespace erwin