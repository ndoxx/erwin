#pragma once

#include "erwin.h"

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
	void on_imgui_render(const erwin::Scene& scene);

private:

};


} // namespace editor