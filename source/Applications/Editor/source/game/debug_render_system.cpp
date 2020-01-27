#include "game/debug_render_system.h"

namespace erwin
{

void DebugRenderSystem::update(const GameClock& clock)
{

}

void DebugRenderSystem::render()
{
	auto& scene = Application::SCENE();

	ForwardRenderer::begin_line_pass(scene.camera_controller.get_camera());
	for(auto&& cmp_tuple: components_)
	{
		ComponentOBB* OBB = eastl::get<ComponentOBB*>(cmp_tuple);
		if(OBB->get_parent_entity() == scene.get_selected_entity())
			ForwardRenderer::draw_cube(glm::scale(OBB->model_matrix, glm::vec3(1.001f)), {1.f,0.5f,0.f});
		else if(OBB->display)
			ForwardRenderer::draw_cube(glm::scale(OBB->model_matrix, glm::vec3(1.001f)), {0.f,0.5f,1.f});
	}
	ForwardRenderer::end_line_pass();
}

} // namespace erwin