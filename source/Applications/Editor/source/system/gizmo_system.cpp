#include "system/gizmo_system.h"
#include "entity/component/tags.h"
#include "level/scene.h"

using namespace erwin;

namespace editor
{

GizmoSystem::GizmoSystem()
{

}

GizmoSystem::~GizmoSystem() {}

void GizmoSystem::setup_editor_entities(erwin::Scene&)
{
    // Create 4 entities with OBBs and transforms that will represent the gizmo's interactive zones
    /*std::array ents = {scene.create_entity(), scene.create_entity(), scene.create_entity(), scene.create_entity()};
    for(size_t ii = 0; ii < ents.size(); ++ii)
    {
        scene.add_component<GizmoHandleComponent>(ents[ii], GizmoHandleComponent{int(ii), k_invalid_entity_id});
        scene.add_component<ComponentOBB>(ents[ii], OBB_extent);
        scene.add_component<ComponentTransform3D>(ents[ii], offsets[ii], glm::vec3(0.f), 1.f);
        scene.add_component<NonSerializableTag>(ents[ii]);
        scene.add_component<HiddenTag>(ents[ii]);
    }*/

    // On object selection, create a Gizmo component
    
    //scene.on_construct<SelectedTag>().connect<&entt::registry::emplace_or_replace<GizmoDirtyTag>>();
    //scene.on_destroy<SelectedTag>().connect<...>();
}

void GizmoSystem::update(const erwin::GameClock&, erwin::Scene&)
{
    // Make the gizmo's entities children of the selected entity
    /*scene.view<ComponentTransform3D, SelectedTag, GizmoDirtyTag>(entt::exclude<NoGizmoTag>)
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
        [this](auto, const auto& GH) { selected_part_ = GH.handle_id; });*/
}

void GizmoSystem::on_imgui_render(const erwin::Scene&)
{
    /*scene.view<const ComponentTransform3D, const SelectedTag>().each([this](auto, const auto& transform) {
        // Draw gizmo
        gizmo_data_.selected = selected_part_;

        Renderer3D::begin_line_pass(false);
        Renderer3D::draw_mesh(CommonGeometry::get_mesh("origin_lines"_h), transform.global.get_model_matrix(),
                              gizmo_material_, &gizmo_data_);
        Renderer3D::end_line_pass();
    });*/
}

} // namespace editor