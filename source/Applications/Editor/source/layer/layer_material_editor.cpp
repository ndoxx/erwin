#include "layer/layer_material_editor.h"
#include "widget/widget_material_view.h"
#include "input/input.h"

using namespace erwin;

namespace editor
{

MaterialEditorLayer::MaterialEditorLayer():
GuiLayer("MaterialEditorLayer")
{

}

void MaterialEditorLayer::on_attach()
{
	// Build UI
    material_view_widget_ = new MaterialViewWidget();
    add_widget(material_view_widget_);
}

void MaterialEditorLayer::on_detach()
{
	for(Widget* widget: widgets_)
		delete widget; 
}

void MaterialEditorLayer::on_update(GameClock& clock)
{
	for(Widget* widget: widgets_)
		widget->on_update(clock);
}

void MaterialEditorLayer::on_render()
{
	for(Widget* widget: widgets_)
		widget->on_layer_render();
}

void MaterialEditorLayer::on_imgui_render()
{
	for(Widget* widget: widgets_)
		widget->imgui_render();
}

bool MaterialEditorLayer::on_event(const MouseButtonEvent& event)
{
    // If scene view is not hovered, let ImGui consume input events
    if(!material_view_widget_->is_hovered())
    {
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureMouse)
            return true;
    }

    return material_view_widget_->on_event(event);
}

bool MaterialEditorLayer::on_event(const MouseMovedEvent& event)
{
    return material_view_widget_->on_event(event);
}

bool MaterialEditorLayer::on_event(const WindowResizeEvent& event)
{
    return material_view_widget_->on_event(event);
}

bool MaterialEditorLayer::on_event(const MouseScrollEvent& event)
{
    if(!material_view_widget_->is_hovered())
    {
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureMouse)
            return true;
    }

    return material_view_widget_->on_event(event);
}

bool MaterialEditorLayer::on_event(const KeyboardEvent& event)
{
    // If scene view is not hovered, let ImGui consume input events
    if(!material_view_widget_->is_hovered())
    {
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureKeyboard)
            return true;
    }

    return material_view_widget_->on_event(event);
}

bool MaterialEditorLayer::on_event(const KeyTypedEvent&)
{
    // Don't propagate event if ImGui wants to handle it
    ImGuiIO& io = ImGui::GetIO();
    return io.WantTextInput;
}

} // namespace editor
