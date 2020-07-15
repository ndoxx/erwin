#include "system/gizmo_system.h"
#include "asset/bounding.h"
#include "entity/component/bounding_box.h"
#include "entity/component/camera.h"
#include "entity/component/hierarchy.h"
#include "entity/component/tags.h"
#include "entity/component/transform.h"
#include "entity/tag_components.h"
#include "render/renderer_3d.h"
#include "level/scene.h"

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
    gizmo_material_ = {"gizmo"_h, {}, gizmo_shader, gizmo_ubo, sizeof(GizmoData), 0};
    Renderer3D::register_shader(gizmo_shader);
    Renderer::shader_attach_uniform_buffer(gizmo_shader, gizmo_ubo);
}

GizmoSystem::~GizmoSystem() {}

void GizmoSystem::setup_editor_entities(erwin::Scene& scene)
{
    // Create 4 entities with OBBs and transforms that will represent the gizmo's interactive zones
    std::array ents = {scene.create_entity(), scene.create_entity(), scene.create_entity(), scene.create_entity()};
    for(size_t ii = 0; ii < ents.size(); ++ii)
    {
        scene.add_component<GizmoHandleComponent>(ents[ii], GizmoHandleComponent{int(ii), k_invalid_entity_id});
        scene.add_component<ComponentOBB>(ents[ii], OBB_extent);
        scene.add_component<ComponentTransform3D>(ents[ii], offsets[ii], glm::vec3(0.f), 1.f);
        scene.add_component<NonSerializableTag>(ents[ii]);
        scene.add_component<HiddenTag>(ents[ii]);
    }

    // On object selection, tag selected entity such that the gizmo will be updated
    scene.on_construct<SelectedTag>().connect<&entt::registry::emplace_or_replace<GizmoDirtyTag>>();
}

void GizmoSystem::update(const erwin::GameClock& /*clock*/, erwin::Scene& scene)
{
    // Make the gizmo's entities children of the selected entity
    scene.view<ComponentTransform3D, SelectedTag, GizmoDirtyTag>(entt::exclude<NoGizmoTag>)
        .each([&scene](auto e, const auto&) {
            scene.view<GizmoHandleComponent>().each([e, &scene](auto e_gh, auto& GH) {
                GH.parent = e;
                scene.attach(e, e_gh);
            });
            scene.try_add_component<DirtyTransformTag>(e);
        });

    scene.clear<GizmoDirtyTag>();

    selected_part_ = -1;
    scene.view<const GizmoHandleComponent, const GizmoHandleSelectedTag>().each(
        [this](auto /*e*/, const auto& GH) { selected_part_ = GH.handle_id; });
}

void GizmoSystem::render(const erwin::Scene& scene)
{
    scene.view<const ComponentTransform3D, const SelectedTag>().each([this](auto /*e*/, const auto& transform) {
        // Draw gizmo
        gizmo_data_.selected = selected_part_;

        Renderer3D::begin_line_pass(false);
        Renderer3D::draw_mesh(CommonGeometry::get_mesh("origin_lines"_h), transform.global.get_model_matrix(),
                              gizmo_material_, &gizmo_data_);
        Renderer3D::end_line_pass();
    });
}

} // namespace editor