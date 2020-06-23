#pragma once

#include "entt/entt.hpp"

namespace erwin
{

class GameClock;
class TransformSystem
{
public:
	void update(const GameClock& clock, entt::registry& registry);
};


} // namespace erwin