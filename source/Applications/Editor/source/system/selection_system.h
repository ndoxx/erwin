#pragma once

#include "erwin.h"

namespace erwin
{
class Scene;
}

namespace editor
{

class SelectionSystem
{
public:
	void update(const erwin::GameClock& clock, erwin::Scene& scene);
};


} // namespace editor