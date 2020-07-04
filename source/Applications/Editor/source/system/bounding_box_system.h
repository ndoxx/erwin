#pragma once

#include "erwin.h"
#include "event/window_events.h"

namespace editor
{

class BoundingBoxSystem
{
public:
    void update(const erwin::GameClock& clock, entt::registry& registry);
    void render(const entt::registry& registry);
    bool on_ray_scene_query_event(const erwin::RaySceneQueryEvent& event);
};

} // namespace editor