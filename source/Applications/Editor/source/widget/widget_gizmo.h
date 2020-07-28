#pragma once

#include "widget/widget.h"

namespace erwin
{
class Scene;
}

namespace editor
{

class GizmoWidget : public Widget
{
public:
    GizmoWidget();
    virtual ~GizmoWidget() = default;

    void setup_editor_entities(erwin::Scene& scene);
    void cycle_operation();
    bool is_in_use() const;
    bool is_hovered() const;

    virtual void on_update(const erwin::GameClock& clock) override;
    virtual void on_imgui_render() override;

private:
    enum class RefFrame : uint8_t
    {
        Local = 0,
        World,

        Count
    };

    enum class Operation : uint8_t
    {
        Disabled = 0,
        Translate,
        Rotate,

        Count
    };

    struct RenderSurface
    {
        float x;
        float y;
        float w;
        float h;
    };

    bool on_framebuffer_resize_event(const erwin::FramebufferResizeEvent&);
    bool on_window_moved_event(const erwin::WindowMovedEvent&);
    void set_operation(Operation op);

private:
    RenderSurface render_surface_;
    RefFrame current_mode_;
    Operation current_operation_;
    bool use_snap_;
    bool show_grid_;
    int grid_size_;
    glm::vec3 snap_;
    glm::mat4 grid_model_;
};

} // namespace editor