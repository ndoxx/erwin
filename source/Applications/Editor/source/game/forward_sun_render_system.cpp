#include "game/forward_sun_render_system.h"

namespace erwin
{

void ForwardSunRenderSystem::update(const GameClock& clock)
{
	for(auto&& cmp_tuple: components_)
	{
		ComponentDirectionalLight* dirlight = eastl::get<ComponentDirectionalLight*>(cmp_tuple);
		ComponentRenderableDirectionalLight* renderable = eastl::get<ComponentRenderableDirectionalLight*>(cmp_tuple);
		renderable->material_data.color = glm::vec4(dirlight->color, 1.f);
		renderable->material_data.brightness = dirlight->brightness;
	}
}

void ForwardSunRenderSystem::render()
{
	VertexArrayHandle quad = CommonGeometry::get_vertex_array("quad"_h);

	auto& scene = Application::SCENE();
	for(auto&& cmp_tuple: components_)
	{
		ComponentDirectionalLight* dirlight = eastl::get<ComponentDirectionalLight*>(cmp_tuple);
		ComponentRenderableDirectionalLight* renderable = eastl::get<ComponentRenderableDirectionalLight*>(cmp_tuple);
		ForwardRenderer::begin_pass(scene.camera_controller.get_camera(), *dirlight);
		ForwardRenderer::draw_mesh(quad, ComponentTransform3D(), renderable->material);
		ForwardRenderer::end_pass();
	}
}

} // namespace erwin