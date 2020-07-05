#include "layer/layer_scene_editor.h"
#include "input/input.h"
#include "level/scene_manager.h"
#include "entity/tag_components.h"
#include "widget/widget_hex_dump.h"
#include "widget/widget_inspector.h"
#include "widget/widget_rt_peek.h"
#include "widget/widget_scene_hierarchy.h"
#include "widget/widget_scene_view.h"

using namespace erwin;

namespace editor
{

SceneEditorLayer::SceneEditorLayer() : GuiLayer("SceneEditorLayer") {}

void SceneEditorLayer::on_attach()
{
    // Reflect editor components here if any

    // Build UI
#ifdef W_DEBUG
    HexDumpWidget* hex_widget;
    add_widget(hex_widget = new HexDumpWidget());
    hex_widget->refresh();
#endif

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

void SceneEditorLayer::on_commit()
{
    add_listener(this, &SceneEditorLayer::on_mouse_button_event);
    add_listener(this, &SceneEditorLayer::on_mouse_scroll_event);
    add_listener(this, &SceneEditorLayer::on_keyboard_event);
    add_listener(this, &SceneEditorLayer::on_key_typed_event);

    add_listener(&bounding_box_system_, &BoundingBoxSystem::on_ray_scene_query_event);
}

void SceneEditorLayer::on_detach()
{
    for(Widget* widget : widgets_)
        delete widget;
}

void SceneEditorLayer::setup_editor_entities(entt::registry& registry)
{
    gizmo_system_.setup_editor_entities(registry);
}

void SceneEditorLayer::on_update(GameClock& clock)
{
    auto& scene = scn::current();
    if(scene.is_loaded())
    {
        bounding_box_system_.update(clock, scene.registry);
        selection_system_.update(clock, scene);
        gizmo_system_.update(clock, scene.registry);
    }

    for(Widget* widget : widgets_)
        widget->on_update(clock);
}

void SceneEditorLayer::on_render()
{
    auto& scene = scn::current();
    if(scene.is_loaded())
    {
        bounding_box_system_.render(scene.registry);
        gizmo_system_.render(scene.registry);
    }

    for(Widget* widget : widgets_)
        widget->on_layer_render();
}

void SceneEditorLayer::on_imgui_render()
{
    for(Widget* widget : widgets_)
        widget->imgui_render();
}

bool SceneEditorLayer::on_mouse_button_event(const MouseButtonEvent& event)
{
    if(!enabled_)
        return false;
    // If scene view is not hovered, let ImGui consume input events
    if(!scene_view_widget_->is_hovered())
    {
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureMouse)
            return true;
    }

    return scene_view_widget_->on_mouse_event(event);
}

bool SceneEditorLayer::on_mouse_scroll_event(const MouseScrollEvent&)
{
    if(!enabled_)
        return false;
    // If scene view is not hovered, let ImGui consume input events
    if(!scene_view_widget_->is_hovered())
    {
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureMouse)
            return true;
    }
    
    return false;
}

bool SceneEditorLayer::on_keyboard_event(const KeyboardEvent& event)
{
    if(!enabled_)
        return false;
    // If scene view is not hovered, let ImGui consume input events
    if(!scene_view_widget_->is_hovered())
    {
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureKeyboard)
            return true;
    }

    if(Input::match_action(ACTION_DROP_SELECTION, event))
    {
        scn::current().registry.clear<SelectedTag>();
        return true;
    }

    return false;
}

bool SceneEditorLayer::on_key_typed_event(const KeyTypedEvent&)
{
    if(!enabled_)
        return false;
    // Don't propagate event if ImGui wants to handle it
    ImGuiIO& io = ImGui::GetIO();
    return io.WantTextInput;
}

} // namespace editor
