#include "game/pbr_deferred_render_system.h"

namespace erwin
{

void PBRDeferredRenderSystem::update(const GameClock& clock)
{

}

void PBRDeferredRenderSystem::render()
{
	Entity& dirlight_ent = manager_->get_entity(p_scene_->directional_light);
	auto* dirlight = dirlight_ent.get_component<ComponentDirectionalLight>();

	DeferredRenderer::begin_pass(p_scene_->camera_controller.get_camera(), *dirlight);
	for(auto&& cmp_tuple: components_)
	{
		ComponentTransform3D* transform = eastl::get<ComponentTransform3D*>(cmp_tuple);
		ComponentRenderablePBR* renderable = eastl::get<ComponentRenderablePBR*>(cmp_tuple);
		DeferredRenderer::draw_mesh(renderable->vertex_array, *transform, renderable->material);
	}
	DeferredRenderer::end_pass();
}

} // namespace erwin