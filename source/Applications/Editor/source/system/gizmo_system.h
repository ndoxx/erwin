#pragma once

#include "erwin.h"
#include "level/scene.h"
#include "event/window_events.h"

namespace erwin
{

class GizmoSystem
{
public:
	GizmoSystem();
	~GizmoSystem();

	void render();
	bool on_ray_scene_query_event(const RaySceneQueryEvent& event);

private:
    erwin::Material gizmo_material_;
    int selected_part_;

    struct GizmoData
    {
    	int selected;
    } gizmo_data_;
};


} // namespace erwin