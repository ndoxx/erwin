#include "system/gizmo_system.h"
#include "core/application.h"
#include "entity/component/tags.h"
#include "entity/component/camera.h"
#include "entity/component/editor_tags.h"
#include "entity/component/gizmo.h"
#include "entity/component/transform.h"
#include "entity/component/hierarchy.h"
#include "event/event_bus.h"
#include "event/window_events.h"
#include "level/scene.h"

#include "imguizmo/ImGuizmo.h"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtx/string_cast.hpp"

using namespace erwin;

namespace editor
{

// TMP: to match with offset in Scene View Widget
static constexpr float k_start_x = 4.f;
static constexpr float k_start_y = 43.f;

GizmoSystem::GizmoSystem():
current_mode_(ImGuizmo::WORLD),
current_operation_(ImGuizmo::ROTATE),
use_snap_(false),
snap_(0.5f)
{
    Application::get_instance().set_on_imgui_newframe_callback([]() { ImGuizmo::BeginFrame(); });

    EventBus::subscribe(this, &GizmoSystem::on_framebuffer_resize_event);
    EventBus::subscribe(this, &GizmoSystem::on_window_moved_event);
}

GizmoSystem::~GizmoSystem() {}

void on_select(entt::registry& reg, entt::entity e)
{
    const auto& ctrans = reg.get<ComponentTransform3D>(e);
    auto& cgizmo = reg.emplace_or_replace<ComponentGizmo>(e);
    cgizmo.model_matrix = ctrans.global.get_model_matrix();
}

void GizmoSystem::setup_editor_entities(Scene& scene)
{
    // On object selection, create a Gizmo component
    // Remove it on object deselection
    scene.on_construct<SelectedTag>().connect<&on_select>();
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
    scene.view<ComponentTransform3D, ComponentGizmo, GizmoDirtyTag>().each(
        [this](auto, auto& transform, auto& gizmo)
    {
        if(current_operation_ == ImGuizmo::TRANSLATE)
        {
            // Delta is in world coordinates, transform back to local coordinates
            transform.local.position += (transform.local.uniform_scale/transform.global.uniform_scale) * glm::vec3(glm::column(gizmo.delta,3));
        }
        else if(current_operation_ == ImGuizmo::ROTATE)
        {
            glm::vec3 trans, rot, scale;
            ImGuizmo::DecomposeMatrixToComponents(&gizmo.model_matrix[0][0], &trans[0], &rot[0], &scale[0]);
            transform.local.set_rotation({rot.x, rot.y, rot.z});
        }
    });
}

void GizmoSystem::on_imgui_render(Scene& scene)
{
    ImGuizmo::SetRect(render_surface_.x, render_surface_.y, render_surface_.w, render_surface_.h);

    auto e_camera = scene.get_named("Camera"_h);
    auto& ccamera = scene.get_component<ComponentCamera3D>(e_camera);

    scene.view<ComponentGizmo>().each([this,&scene,&ccamera](auto e, auto& gizmo) {
        ImGuizmo::Manipulate(&ccamera.view_matrix[0][0], &ccamera.projection_matrix[0][0],
                             ImGuizmo::OPERATION(current_operation_), ImGuizmo::MODE(current_mode_),
                             &gizmo.model_matrix[0][0], &gizmo.delta[0][0], use_snap_ ? &snap_[0] : nullptr);
        if(ImGuizmo::IsUsing())
        {
            scene.try_add_component<DirtyTransformTag>(e);
            scene.try_add_component<GizmoDirtyTag>(e);
        }
    });
}

} // namespace editor