#pragma once

#include "erwin.h"

namespace editor
{

class Scene;
class SelectionSystem
{
public:
	void update(const erwin::GameClock& clock, Scene& scene);
};


} // namespace editor