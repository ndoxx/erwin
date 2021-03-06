#pragma once

#include "core/layer.h"
#include "entity/system/transform_hierarchy_system.h"
#include "input/freefly_camera_system.h"

namespace erwin
{
class Scene;
}

namespace editor
{

class SceneViewLayer : public erwin::Layer
{
public:
    SceneViewLayer(erwin::Application& application);
    ~SceneViewLayer() = default;

    virtual void on_imgui_render() override;

    void setup_camera(erwin::Scene& scene);

protected:
    virtual void on_attach() override;
    virtual void on_detach() override;
    virtual void on_update(erwin::GameClock& clock) override;
    virtual void on_render() override;
    virtual void on_commit() override;

    bool on_mouse_button_event(const erwin::MouseButtonEvent& event);
    bool on_mouse_moved_event(const erwin::MouseMovedEvent& event);
    bool on_window_resize_event(const erwin::WindowResizeEvent& event);
    bool on_window_moved_event(const erwin::WindowMovedEvent& event);
    bool on_mouse_scroll_event(const erwin::MouseScrollEvent& event);
    bool on_keyboard_event(const erwin::KeyboardEvent& event);

private:
    erwin::FreeflyCameraSystem camera_controller_;
    erwin::TransformSystem transform_system_;
    bool freefly_mode_ = false;
};

} // namespace editor