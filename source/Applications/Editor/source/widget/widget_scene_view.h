#pragma once

#include "widget/widget.h"

namespace erwin
{
class Scene;
}

namespace editor
{

struct RenderSurface
{
    float x0;
    float y0;
    float x1;
    float y1;
    float w;
    float h;
};

class GizmoOverlay;
class SceneViewWidget : public Widget
{
public:
    SceneViewWidget(erwin::EventBus&);
    virtual ~SceneViewWidget();

    virtual void on_update(const erwin::GameClock& clock) override;

    bool on_mouse_event(const erwin::MouseButtonEvent& event);
    bool on_keyboard_event(const erwin::KeyboardEvent& event);
    void setup_editor_entities(erwin::Scene& scene);

    void runtime_start();
    void runtime_stop();
    void runtime_pause();
    void runtime_reset();

protected:
    virtual void on_imgui_render() override;
    virtual void on_resize(uint32_t width, uint32_t height) override;
    virtual void on_move(int32_t x_pos, int32_t y_pos) override;

    void frame_profiler_window(bool* p_open);

private:
    Widget* stats_overlay_;
    GizmoOverlay* gizmo_overlay_;

    RenderSurface render_surface_;
    bool enable_runtime_profiling_;
    bool track_next_frame_draw_calls_;
    bool runtime_;
    bool paused_;
};

} // namespace editor