#pragma once

#include "erwin.h"
#include "event/window_events.h"

namespace editor
{

class BoundingBoxSystem
{
public:
	void update(const erwin::GameClock& clock);
	void render();
	bool on_ray_scene_query_event(const erwin::RaySceneQueryEvent& event);
};


} // namespace editor