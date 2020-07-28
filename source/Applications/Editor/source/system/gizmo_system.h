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
	void on_imgui_render(erwin::Scene& scene);

private:
    bool on_framebuffer_resize_event(const erwin::FramebufferResizeEvent&);
    bool on_window_moved_event(const erwin::WindowMovedEvent&);

    struct RenderSurface
    {
        float x;
        float y;
        float w;
        float h;
    };
    RenderSurface render_surface_;
    int current_mode_;
    int current_operation_;
    bool use_snap_;
    glm::vec3 snap_;
};


} // namespace editor