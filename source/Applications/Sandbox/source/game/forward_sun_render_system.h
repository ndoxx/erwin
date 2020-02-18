#pragma once

#include "erwin.h"
#include "level/scene.h"
#include "game/game_components.h"

namespace erwin
{

class ForwardSunRenderSystem
{
public:
	void render()
	{
		VertexArrayHandle quad = CommonGeometry::get_vertex_array("quad"_h);

		Renderer3D::begin_forward_pass();
	    auto view = Scene::registry.view<ComponentDirectionalLight,ComponentRenderableDirectionalLight>();
	    for(const entt::entity e: view)
	    {
	        const ComponentDirectionalLight& dirlight = view.get<ComponentDirectionalLight>(e);
	        ComponentRenderableDirectionalLight& renderable = view.get<ComponentRenderableDirectionalLight>(e);

			renderable.material_data.color = glm::vec4(dirlight.color, 1.f);
			renderable.material_data.brightness = dirlight.brightness;
			renderable.material.data = &renderable.material_data; // Dirty shit

			Renderer3D::draw_mesh(quad, glm::mat4(1.f), renderable.material);
		}
		Renderer3D::end_forward_pass();
	}
};


} // namespace erwin