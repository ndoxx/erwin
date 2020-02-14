#pragma once

#include "erwin.h"
#include "event/window_events.h"

namespace erwin
{

class BoundingBoxSystem
{
public:
	BoundingBoxSystem();

	void update(const GameClock& clock);
	void render();
	bool on_ray_scene_query_event(const RaySceneQueryEvent& event);
};


} // namespace erwin