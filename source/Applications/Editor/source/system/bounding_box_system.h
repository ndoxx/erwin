#pragma once

#include "erwin.h"
#include "event/window_events.h"

namespace editor
{

class Scene;
class BoundingBoxSystem
{
public:
	void update(const erwin::GameClock& clock, Scene& scene);
	void render(const Scene& scene);
	bool on_ray_scene_query_event(const erwin::RaySceneQueryEvent& event);
};


} // namespace editor