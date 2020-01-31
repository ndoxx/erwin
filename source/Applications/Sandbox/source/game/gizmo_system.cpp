#include "game/gizmo_system.h"

namespace erwin
{

GizmoSystem::GizmoSystem(EntityManager* manager):
BaseType(manager)
{
    gizmo_shader_ = Renderer::create_shader(filesystem::get_system_asset_dir() / "shaders/gizmo.glsl", "gizmo");
    ForwardRenderer::register_shader(gizmo_shader_);
}

GizmoSystem::~GizmoSystem()
{
    Renderer::destroy(gizmo_shader_);
}

void GizmoSystem::update(const GameClock& clock)
{

}

void GizmoSystem::render()
{
    auto& scene = Application::SCENE();
    auto& ecs   = Application::ECS();
    // TODO: handle the "nothing selected" case...
    auto& selected_entity = ecs.get_entity(scene.get_selected_entity());
    auto* transform = selected_entity.get_component<ComponentTransform3D>();
    if(transform == nullptr)
        return;

    // Draw gizmo
    ForwardRenderer::begin_line_pass(false);
    ForwardRenderer::draw_mesh(CommonGeometry::get_vertex_array("origin_lines"_h), transform->get_unscaled_model_matrix(), Material{gizmo_shader_});
    ForwardRenderer::end_line_pass();
}

} // namespace erwin