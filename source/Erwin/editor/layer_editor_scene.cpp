#include "editor/layer_editor_scene.h"
#include "editor/editor_components.h"
#include "editor/widget_scene_view.h"
#include "editor/widget_scene_hierarchy.h"
#include "editor/widget_inspector.h"
#include "editor/widget_rt_peek.h"
#include "editor/widget_hex_dump.h"
#include "editor/widget_materials.h"
#include "input/input.h"
#include "level/scene.h"

using namespace erwin;

namespace editor
{

SceneEditorLayer::SceneEditorLayer():
Layer("SceneEditorLayer")
{

}

void SceneEditorLayer::add_widget(Widget* widget)
{
	widgets_.push_back(widget);
}

void SceneEditorLayer::on_attach()
{
    // Reflect editor components
    REFLECT_COMPONENT(ComponentEditorDescription);

	// Load resources
    Scene::init();

	// Build UI
    editor::HexDumpWidget* hex_widget;
    add_widget(hex_widget = new editor::HexDumpWidget());
    hex_widget->refresh();

    scene_view_widget_ = new editor::SceneViewWidget();
    add_widget(scene_view_widget_);
    add_widget(new editor::SceneHierarchyWidget());
    add_widget(new editor::MaterialsWidget());
    add_widget(new editor::InspectorWidget());

    // Register main render targets in peek widget
    editor::RTPeekWidget* peek_widget;
    add_widget(peek_widget = new editor::RTPeekWidget());
	peek_widget->register_framebuffer("GBuffer");
	peek_widget->register_framebuffer("SpriteBuffer");
	peek_widget->register_framebuffer("BloomCombine");
	peek_widget->register_framebuffer("LBuffer");

    DLOGN("editor") << "Erwin Editor is ready." << std::endl;
}

void SceneEditorLayer::on_detach()
{
	for(Widget* widget: widgets_)
		delete widget;

    Scene::shutdown();    
}

void SceneEditorLayer::on_update(GameClock& clock)
{
    bounding_box_system_.update(clock);
    
	for(Widget* widget: widgets_)
		widget->on_update();
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
