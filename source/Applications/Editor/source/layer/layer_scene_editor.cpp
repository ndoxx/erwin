#include "layer/layer_scene_editor.h"
#include "widget/widget_scene_view.h"
#include "widget/widget_scene_hierarchy.h"
#include "widget/widget_inspector.h"
#include "widget/widget_rt_peek.h"
#include "widget/widget_hex_dump.h"
#include "input/input.h"
#include "level/scene.h"

using namespace erwin;

namespace editor
{

SceneEditorLayer::SceneEditorLayer():
GuiLayer("SceneEditorLayer")
{

}

void SceneEditorLayer::on_attach()
{
    // Reflect editor components
    REFLECT_COMPONENT(ComponentDescription);

	// Build UI
    HexDumpWidget* hex_widget;
    add_widget(hex_widget = new HexDumpWidget());
    hex_widget->refresh();

    scene_view_widget_ = new SceneViewWidget();
    add_widget(scene_view_widget_);
    add_widget(new SceneHierarchyWidget());
    add_widget(new InspectorWidget());

    // Register main render targets in peek widget
    RTPeekWidget* peek_widget;
    add_widget(peek_widget = new RTPeekWidget());
	peek_widget->register_framebuffer("GBuffer");
	peek_widget->register_framebuffer("SpriteBuffer");
	peek_widget->register_framebuffer("BloomCombine");
	peek_widget->register_framebuffer("LBuffer");
}

void SceneEditorLayer::on_detach()
{
	for(Widget* widget: widgets_)
		delete widget; 
}

void SceneEditorLayer::on_update(GameClock& clock)
{
    bounding_box_system_.update(clock);
    
	for(Widget* widget: widgets_)
		widget->on_update(clock);
}

void SceneEditorLayer::on_render()
{
    bounding_box_system_.render();
    gizmo_system_.render();

	for(Widget* widget: widgets_)
		widget->on_layer_render();
}

void SceneEditorLayer::on_imgui_render()
{
	for(Widget* widget: widgets_)
		widget->imgui_render();
}

bool SceneEditorLayer::on_event(const MouseButtonEvent& event)
{
    // If scene view is not hovered, let ImGui consume input events
    if(!scene_view_widget_->is_hovered())
    {
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureMouse)
            return true;
    }

    return scene_view_widget_->on_mouse_event(event);
}

bool SceneEditorLayer::on_event(const WindowResizeEvent&)
{
	return false;
}

bool SceneEditorLayer::on_event(const MouseScrollEvent&)
{
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

bool SceneEditorLayer::on_event(const KeyboardEvent& event)
{
    // If scene view is not hovered, let ImGui consume input events
    if(!scene_view_widget_->is_hovered())
    {
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureKeyboard)
            return true;
    }

    if(event.pressed && !event.repeat && event.key == Input::get_action_key(ACTION_DROP_SELECTION))
    {
        Scene::drop_selection();
        return true;
    }

    return false;
}

bool SceneEditorLayer::on_event(const KeyTypedEvent&)
{
    // Don't propagate event if ImGui wants to handle it
    ImGuiIO& io = ImGui::GetIO();
    return io.WantTextInput;
}

} // namespace editor
