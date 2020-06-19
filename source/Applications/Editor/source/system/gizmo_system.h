#pragma once

#include "erwin.h"
#include "event/window_events.h"

namespace editor
{

class Scene;
class GizmoSystem
{
public:
	GizmoSystem();
	~GizmoSystem();

    void setup_editor_entities(Scene& scene);

    void update(const erwin::GameClock& clock, Scene& scene);
	void render(const Scene& scene);

private:
    erwin::Material gizmo_material_;
    int selected_part_;

    struct GizmoData
    {
    	int selected;
    } gizmo_data_;
};


} // namespace editor