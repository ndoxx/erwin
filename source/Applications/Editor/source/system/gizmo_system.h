#pragma once

#include "erwin.h"
#include "event/window_events.h"

namespace editor
{

class GizmoSystem
{
public:
	GizmoSystem();
	~GizmoSystem();

    void setup_editor_entities(entt::registry& registry);

    void update(const erwin::GameClock& clock, entt::registry& registry);
	void render(const entt::registry& registry);

private:
    erwin::Material gizmo_material_;
    int selected_part_;

    struct GizmoData
    {
    	int selected;
    } gizmo_data_;
};


} // namespace editor