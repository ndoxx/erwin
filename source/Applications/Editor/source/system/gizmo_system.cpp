#include "system/gizmo_system.h"
#include "core/application.h"
#include "entity/component/camera.h"
#include "entity/component/editor_tags.h"
#include "entity/component/gizmo.h"
#include "entity/component/transform.h"
#include "level/scene.h"
#include "event/event_bus.h"
#include "event/window_events.h"

#include "imguizmo/ImGuizmo.h"

using namespace erwin;

namespace editor
{

// TMP: to match with offset in Scene View Widget
static constexpr float k_start_x = 4.f;
static constexpr float k_start_y = 43.f;

GizmoSystem::GizmoSystem()
{
    Application::get_instance().set_on_imgui_newframe_callback([]() { ImGuizmo::BeginFrame(); });

    EventBus::subscribe(this, &GizmoSystem::on_framebuffer_resize_event);
    EventBus::subscribe(this, &GizmoSystem::on_window_moved_event);
}

GizmoSystem::~GizmoSystem() {}

void GizmoSystem::setup_editor_entities(Scene& scene)
{
    // On object selection, create a Gizmo component
    // Remove it on object deselection
    scene.on_construct<SelectedTag>().connect<&entt::registry::emplace_or_replace<ComponentGizmo>>();
    scene.on_destroy<SelectedTag>().connect<&entt::registry::remove_if_exists<ComponentGizmo>>();
}

bool GizmoSystem::on_framebuffer_resize_event(const FramebufferResizeEvent& event)
{
    render_surface_.w = float(event.width);
    render_surface_.h = float(event.height);
    return false;
}

bool GizmoSystem::on_window_moved_event(const WindowMovedEvent& event)
{
    render_surface_.x = float(event.x) + k_start_x;
    render_surface_.y = float(event.y) + k_start_y;
    return false;
}


void GizmoSystem::update(const GameClock&, Scene& scene)
{
    scene.view<const ComponentTransform3D, ComponentGizmo>().each(
        [](auto, const auto& transform, auto& gizmo) {
            gizmo.model_matrix = transform.global.get_model_matrix();
        });
}

void GizmoSystem::on_imgui_render(Scene& scene)
{
    ImGuizmo::SetRect(render_surface_.x, render_surface_.y, render_surface_.w, render_surface_.h);

    auto e_camera = scene.get_named("Camera"_h);
    auto& ccamera = scene.get_component<ComponentCamera3D>(e_camera);

    scene.view<ComponentGizmo>().each(
        [&ccamera](auto, auto& gizmo) {
            auto gizmo_operation = ImGuizmo::TRANSLATE;
            auto gizmo_mode = ImGuizmo::LOCAL;

            ImGuizmo::Manipulate(&ccamera.view_matrix[0][0], &ccamera.projection_matrix[0][0], gizmo_operation,
                                 gizmo_mode, &gizmo.model_matrix[0][0], nullptr, nullptr);
        });
}

} // namespace editor