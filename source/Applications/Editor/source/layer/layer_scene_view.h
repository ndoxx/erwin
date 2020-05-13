#pragma once

#include "erwin.h"

class SceneViewLayer : public erwin::Layer
{
public:
    friend class Editor;

    SceneViewLayer();
    ~SceneViewLayer() = default;
    
    virtual void on_imgui_render() override;

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
};