#include "widget/overlay_gizmo.h"
#include "core/application.h"
#include "entity/component/camera.h"
#include "entity/component/editor_tags.h"
#include "entity/component/gizmo.h"
#include "entity/component/hierarchy.h"
#include "entity/component/tags.h"
#include "entity/component/transform.h"
#include "event/event_bus.h"
#include "event/window_events.h"
#include "level/scene_manager.h"
#include "widget/widget_scene_view.h"

#include "glm/gtc/matrix_access.hpp"
#include "glm/gtx/string_cast.hpp"
#include "imgui.h"
#include "imguizmo/ImGuizmo.h"

#include <array>

using namespace erwin;

namespace editor
{

static constexpr std::array<ImGuizmo::OPERATION, 3> k_ops = {ImGuizmo::TRANSLATE, ImGuizmo::TRANSLATE,
                                                             ImGuizmo::ROTATE};
static constexpr std::array<ImGuizmo::MODE, 2> k_modes = {ImGuizmo::LOCAL, ImGuizmo::WORLD};

GizmoOverlay::GizmoOverlay(erwin::EventBus& event_bus)
    : Widget("Manipulator", true, event_bus), current_mode_(RefFrame::World), current_operation_(Operation::Translate),
      use_snap_(true), show_grid_(false), grid_size_(8), snap_(0.5f), grid_model_(1.f)
{
    flags_ = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
             ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav;

    Application::get_instance().set_on_imgui_newframe_callback([]() { ImGuizmo::BeginFrame(); });
}

void on_select(entt::registry& reg, entt::entity e)
{
    if(reg.has<NoGizmoTag>(e) || !reg.has<ComponentTransform3D>(e))
        return;
    const auto& ctrans = reg.get<ComponentTransform3D>(e);
    auto& cgizmo = reg.emplace_or_replace<ComponentGizmo>(e);
    cgizmo.model_matrix = ctrans.global.get_model_matrix();
    cgizmo.delta = glm::mat4(0.f);
}

void GizmoOverlay::setup_editor_entities(Scene& scene)
{
    // On object selection, create a Gizmo component
    // Remove it on object deselection
    scene.on_construct<SelectedTag>().connect<&on_select>();
    scene.on_destroy<SelectedTag>().connect<&entt::registry::remove_if_exists<ComponentGizmo>>();
}

void GizmoOverlay::cycle_operation()
{
    set_operation(Operation((uint8_t(current_operation_) + 1) % uint8_t(Operation::Count)));
}

void GizmoOverlay::set_operation(Operation op)
{
    current_operation_ = op;
    if(current_operation_ == Operation::Translate)
        snap_ = {0.5f, 0.5f, 0.5f};
    else if(current_operation_ == Operation::Rotate)
        snap_.x = 45.f;
}

bool GizmoOverlay::is_in_use() const { return ImGuizmo::IsUsing(); }

bool GizmoOverlay::is_hovered() const { return ImGuizmo::IsOver(); }

void GizmoOverlay::on_update(const GameClock&)
{
    if(current_operation_ == Operation::Disabled)
        return;

    auto& scene = scn::current();

    scene.view<ComponentTransform3D, ComponentGizmo, GizmoDirtyTag>().each(
        [](auto, auto& transform, auto& gizmo) { gizmo.model_matrix = transform.global.get_model_matrix(); });
    scene.clear<GizmoDirtyTag>();

    scene.view<ComponentTransform3D, ComponentGizmo, GizmoUpdateTag>().each([this](auto, auto& transform, auto& gizmo) {
        if(current_operation_ == Operation::Translate)
        {
            // Delta is in world coordinates, transform back to local coordinates
            transform.local.position += (transform.local.uniform_scale / transform.global.uniform_scale) *
                                        glm::vec3(glm::column(gizmo.delta, 3));
        }
        else if(current_operation_ == Operation::Rotate)
        {
            glm::vec3 trans, rot, scale;
            ImGuizmo::DecomposeMatrixToComponents(&gizmo.model_matrix[0][0], &trans[0], &rot[0], &scale[0]);
            transform.local.set_rotation({rot.x, rot.y, rot.z});
        }
    });
}

void GizmoOverlay::on_imgui_render()
{
    // * Interface
    if(ImGui::RadioButton("Dis", current_operation_ == Operation::Disabled))
        set_operation(Operation::Disabled);

    ImGui::SameLine();
    if(ImGui::RadioButton("Tra", current_operation_ == Operation::Translate))
        set_operation(Operation::Translate);

    ImGui::SameLine();
    if(ImGui::RadioButton("Rot", current_operation_ == Operation::Rotate))
        set_operation(Operation::Rotate);

    if(current_operation_ != Operation::Disabled)
    {
        ImGui::TextUnformatted("Snap");
        ImGui::Checkbox("##GizmoSnap", &use_snap_);
        ImGui::SameLine();
        switch(current_operation_)
        {
        case Operation::Translate:
            ImGui::InputFloat3("##GizmoSnapValue", &snap_.x);
            break;
        case Operation::Rotate:
            ImGui::InputFloat("##GizmoSnapValue", &snap_.x);
            break;
        default:
            break;
        }
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Grid");
    ImGui::Checkbox("##ShowGrid", &show_grid_);
    ImGui::SameLine();
    ImGui::SliderInt("##GridSize", &grid_size_, 1, 16);
}

void GizmoOverlay::draw_gizmo(const RenderSurface& rs)
{
    // * Manipulation
    auto& scene = scn::current();
    if(!scene.is_loaded() || current_operation_ == Operation::Disabled || scene.is_runtime())
        return;

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(rs.x0, rs.y0, rs.w, rs.h);

    auto e_camera = scene.get_named("Camera"_h);
    auto& ccamera = scene.get_component<ComponentCamera3D>(e_camera);

    scene.view<ComponentGizmo>().each([this, &scene, &ccamera](auto e, auto& gizmo) {
        ImGuizmo::Manipulate(&ccamera.view_matrix[0][0], &ccamera.projection_matrix[0][0],
                             k_ops[size_t(current_operation_)], k_modes[size_t(current_mode_)],
                             &gizmo.model_matrix[0][0], &gizmo.delta[0][0], use_snap_ ? &snap_[0] : nullptr);
        if(ImGuizmo::IsUsing())
        {
            scene.try_add_component<DirtyTransformTag>(e);
            scene.try_add_component<GizmoUpdateTag>(e);
        }
    });

    // TODO: Grid should use depth test, maybe configure a callback in ImGui draw list to change this state?
    if(show_grid_)
        ImGuizmo::DrawGrid(&ccamera.view_matrix[0][0], &ccamera.projection_matrix[0][0], &grid_model_[0][0],
                           float(grid_size_));

    /*{
        float vmw = 100.f;
        float startx = rs.x1 - vmw;
        float starty = rs.y0;
        ImGuizmo::ViewManipulate(&ccamera.view_matrix[0][0], 50.f, {startx, starty}, {vmw, vmw}, 0xCCCCCCCC);
    }*/
}

} // namespace editor