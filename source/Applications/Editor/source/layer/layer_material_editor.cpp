#include "layer/layer_material_editor.h"
#include "input/input.h"
#include "widget/widget_material_authoring.h"
#include "widget/widget_material_view.h"

using namespace erwin;

namespace editor
{

MaterialEditorLayer::MaterialEditorLayer() : GuiLayer("MaterialEditorLayer") {}

void MaterialEditorLayer::on_attach()
{
    // Build UI
    material_view_widget_ = new MaterialViewWidget();
    material_authoring_widget_ = new MaterialAuthoringWidget(*material_view_widget_);
    add_widget(material_view_widget_);
    add_widget(material_authoring_widget_);
}

void MaterialEditorLayer::on_commit()
{
    add_listener(this, &MaterialEditorLayer::on_mouse_button_event);
    add_listener(this, &MaterialEditorLayer::on_mouse_moved_event);
    add_listener(this, &MaterialEditorLayer::on_window_resize_event);
    add_listener(this, &MaterialEditorLayer::on_mouse_scroll_event);
    add_listener(this, &MaterialEditorLayer::on_keyboard_event);
    add_listener(this, &MaterialEditorLayer::on_key_typed_event);
}

void MaterialEditorLayer::on_detach()
{
    for(Widget* widget : widgets_)
        delete widget;
}

void MaterialEditorLayer::on_update(GameClock& clock)
{
    for(Widget* widget : widgets_)
        widget->on_update(clock);
}

void MaterialEditorLayer::on_render()
{
    for(Widget* widget : widgets_)
        widget->on_layer_render();
}

void MaterialEditorLayer::on_imgui_render()
{
    for(Widget* widget : widgets_)
        widget->imgui_render();
}

bool MaterialEditorLayer::on_mouse_button_event(const MouseButtonEvent& event)
{
    if(!enabled_)
        return false;

    // If material view is not hovered, let ImGui consume input events
    if(!material_view_widget_->is_hovered())
    {
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureMouse)
            return true;
    }

    return material_view_widget_->on_event(event);
}

bool MaterialEditorLayer::on_mouse_moved_event(const MouseMovedEvent& event)
{
    if(!enabled_)
        return false;
    return material_view_widget_->on_event(event);
}

bool MaterialEditorLayer::on_window_resize_event(const WindowResizeEvent& event)
{
    material_view_widget_->on_event(event);
    return false;
}

bool MaterialEditorLayer::on_mouse_scroll_event(const MouseScrollEvent& event)
{
    if(!enabled_)
        return false;
    if(!material_view_widget_->is_hovered())
    {
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureMouse)
            return true;
    }

    return material_view_widget_->on_event(event);
}

bool MaterialEditorLayer::on_keyboard_event(const KeyboardEvent& event)
{
    if(!enabled_)
        return false;
    // If scene view is not hovered, let ImGui consume input events
    if(!material_view_widget_->is_hovered())
    {
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureKeyboard)
            return true;
    }

    return material_view_widget_->on_event(event);
}

bool MaterialEditorLayer::on_key_typed_event(const KeyTypedEvent&)
{
    if(!enabled_)
        return false;
    // Don't propagate event if ImGui wants to handle it
    ImGuiIO& io = ImGui::GetIO();
    return io.WantTextInput;
}

} // namespace editor
