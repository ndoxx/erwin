#pragma once

#include "entt/entt.hpp"

namespace erwin
{

class GameClock;
class Scene;
class TransformSystem
{
public:
	void update(const GameClock& clock, Scene& scene);
};


} // namespace erwin