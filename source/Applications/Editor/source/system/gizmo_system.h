#pragma once

#include "erwin.h"
#include "event/window_events.h"

namespace erwin
{
    class Scene;
}

namespace editor
{

class GizmoSystem
{
public:
	GizmoSystem();
	~GizmoSystem();

    void setup_editor_entities(erwin::Scene& scene);

    void update(const erwin::GameClock& clock, erwin::Scene& scene);
	void render(const erwin::Scene& scene);

private:
    erwin::Material gizmo_material_;
    int selected_part_;

    struct GizmoData
    {
    	int selected;
    } gizmo_data_;
};


} // namespace editor