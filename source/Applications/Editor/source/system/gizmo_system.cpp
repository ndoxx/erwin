#include "system/gizmo_system.h"
#include "asset/bounding.h"
#include "entity/component_bounding_box.h"
#include "entity/component_camera.h"
#include "entity/component_transform.h"
#include "entity/tag_components.h"
#include "level/scene.h"
#include "render/renderer_3d.h"

using namespace erwin;

namespace editor
{

// constexpr float k_cyl_diameter = 0.01f;
constexpr float k_cyl_length = 2.f;
constexpr float k_arrow_diameter = 0.1f;
constexpr float k_arrow_length = 0.3f;
constexpr float k_offset = k_cyl_length + 0.5f * k_arrow_length;
constexpr float k_OBB_scale = 1.5f;

constexpr glm::vec3 offsets[] = {
    {0.f, 0.f, 0.f},
    {k_offset, 0.f, 0.f},
    {0.f, k_offset, 0.f},
    {0.f, 0.f, k_offset},
};
static const Extent OBB_extent(-k_OBB_scale* k_arrow_diameter, k_OBB_scale* k_arrow_diameter,
                               -k_OBB_scale* k_arrow_diameter, k_OBB_scale* k_arrow_diameter,
                               -k_OBB_scale* k_arrow_diameter, k_OBB_scale* k_arrow_diameter);

GizmoSystem::GizmoSystem()
{
    auto gizmo_shader = Renderer::create_shader(wfs::get_system_asset_dir() / "shaders/gizmo.glsl", "gizmo");
    auto gizmo_ubo = Renderer::create_uniform_buffer("gizmo_data", nullptr, sizeof(GizmoData), UsagePattern::Dynamic);
    gizmo_material_ = {"gizmo"_h, {}, gizmo_shader, gizmo_ubo, sizeof(GizmoData)};
    Renderer3D::register_shader(gizmo_shader);
    Renderer::shader_attach_uniform_buffer(gizmo_shader, gizmo_ubo);
}

GizmoSystem::~GizmoSystem() {}

void GizmoSystem::setup_editor_entities(Scene& scene)
{
    // Create 4 entities with OBBs and transforms that will represent the gizmo's interactive zones
    std::array ents = {scene.registry.create(), scene.registry.create(), scene.registry.create(),
                       scene.registry.create()};
    for(size_t ii = 0; ii < ents.size(); ++ii)
    {
        scene.registry.emplace<GizmoHandleComponent>(ents[ii], GizmoHandleComponent{int(ii), k_invalid_entity_id});
        scene.registry.emplace<ComponentOBB>(ents[ii], OBB_extent);
        scene.registry.emplace<ComponentTransform3D>(ents[ii], offsets[ii], glm::vec3(0.f), 1.f);
    }
}

void GizmoSystem::update(const erwin::GameClock& /*clock*/, Scene& scene)
{
    // Make the gizmo's entities children of the selected entity
    // TMP: impl subject to change when we have a proper hierarchy system
    scene.registry.view<ComponentTransform3D, SelectedTag>(entt::exclude<NoGizmoTag>).each([&scene](auto e, const auto& parent_transform) {
        scene.registry.view<ComponentTransform3D, GizmoHandleComponent, ComponentOBB>().each(
            [e, &parent_transform](auto /*g*/, auto& transform, auto& GH, const auto&) {
                GH.parent = e;
                transform.local.init(parent_transform.local.position + offsets[size_t(GH.handle_id)], parent_transform.local.euler, 1.f);
            });
    });

    selected_part_ = -1;
    scene.registry.view<const GizmoHandleComponent, const GizmoHandleSelectedTag>().each(
        [this](auto /*e*/, const auto& GH) { selected_part_ = GH.handle_id; });
}

void GizmoSystem::render(const Scene& scene)
{
    scene.registry.view<const ComponentTransform3D, const SelectedTag>().each(
        [this](auto /*e*/, const auto& transform) {
            // Draw gizmo
            gizmo_data_.selected = selected_part_;

            Renderer3D::begin_line_pass(false);
            // TMP: use global transform
            Renderer3D::draw_mesh(CommonGeometry::get_mesh("origin_lines"_h), transform.local.get_unscaled_model_matrix(),
                                  gizmo_material_, &gizmo_data_);
            Renderer3D::end_line_pass();
        });
}

} // namespace editor