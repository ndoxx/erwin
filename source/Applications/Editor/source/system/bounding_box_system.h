#pragma once

#include "erwin.h"
#include "event/window_events.h"

namespace erwin
{
    class Scene;
}

namespace editor
{

class BoundingBoxSystem
{
public:
    void update(const erwin::GameClock& clock, erwin::Scene& registry);
    void render(const erwin::Scene& scene);
    bool on_ray_scene_query_event(const erwin::RaySceneQueryEvent& event);
};

} // namespace editor