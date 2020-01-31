#include "game/pbr_deferred_render_system.h"

namespace erwin
{

void PBRDeferredRenderSystem::update(const GameClock& clock)
{

}

void PBRDeferredRenderSystem::render()
{
	DeferredRenderer::begin_pass();
	for(auto&& cmp_tuple: components_)
	{
		ComponentTransform3D* transform = eastl::get<ComponentTransform3D*>(cmp_tuple);
		ComponentRenderablePBR* renderable = eastl::get<ComponentRenderablePBR*>(cmp_tuple);
		DeferredRenderer::draw_mesh(renderable->vertex_array, transform->get_model_matrix(), renderable->material);
	}
	DeferredRenderer::end_pass();
}

} // namespace erwin