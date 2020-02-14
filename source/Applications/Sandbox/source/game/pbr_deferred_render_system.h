#pragma once

#include "erwin.h"
#include "editor/scene.h"
#include "game/game_components.h"

namespace erwin
{

class PBRDeferredRenderSystem
{
public:
	void render()
	{
		Renderer3D::begin_deferred_pass();
	    auto view = editor::Scene::registry.view<ComponentTransform3D,ComponentRenderablePBR>();
	    for(const entt::entity e: view)
	    {
	        const ComponentTransform3D& transform = view.get<ComponentTransform3D>(e);
	        ComponentRenderablePBR& renderable = view.get<ComponentRenderablePBR>(e);
			renderable.material.data = &renderable.material_data; // Dirty shit
			Renderer3D::draw_mesh(renderable.vertex_array, transform.get_model_matrix(), renderable.material);
		}
		Renderer3D::end_deferred_pass();
	}
};

} // namespace erwin