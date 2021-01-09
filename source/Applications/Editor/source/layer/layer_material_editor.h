#pragma once

#include "erwin.h"
#include "layer/gui_layer.h"

namespace editor
{

class MaterialViewWidget;
class MaterialAuthoringWidget;
class MaterialEditorLayer : public GuiLayer
{
public:
    MaterialEditorLayer(erwin::Application&);

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
    bool on_mouse_scroll_event(const erwin::MouseScrollEvent& event);
    bool on_keyboard_event(const erwin::KeyboardEvent& event);
    bool on_key_typed_event(const erwin::KeyTypedEvent& event);

private:
    MaterialViewWidget* material_view_widget_ = nullptr;
    MaterialAuthoringWidget* material_authoring_widget_ = nullptr;
};

} // namespace editor
