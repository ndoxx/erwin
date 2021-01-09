#pragma once

#include "layer/gui_layer.h"
#include "system/bounding_box_system.h"
#include "system/selection_system.h"

namespace erwin
{
    class Scene;
}

namespace editor
{

class SceneViewWidget;
class SceneEditorLayer : public GuiLayer
{
public:
    SceneEditorLayer(erwin::Application&);
    
    void setup_editor_entities(erwin::Scene& scene);

    virtual void on_imgui_render() override;

protected:
    virtual void on_attach() override;
    virtual void on_detach() override;
    virtual void on_update(erwin::GameClock& clock) override;
    virtual void on_render() override;
    virtual void on_commit() override;

    bool on_mouse_button_event(const erwin::MouseButtonEvent& event);
    bool on_mouse_scroll_event(const erwin::MouseScrollEvent& event);
    bool on_keyboard_event(const erwin::KeyboardEvent& event);
    bool on_key_typed_event(const erwin::KeyTypedEvent& event);

private:
    BoundingBoxSystem bounding_box_system_;
    SelectionSystem selection_system_;

    SceneViewWidget* scene_view_widget_ = nullptr;
};

} // namespace editor
