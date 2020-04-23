#pragma once

#include "erwin.h"
#include "level/scene.h"

namespace erwin
{

class PBRDeferredRenderSystem
{
public:
	void render()
	{
		Renderer3D::begin_deferred_pass();
	    auto view = Scene::registry.view<ComponentTransform3D,ComponentPBRMaterial,ComponentMesh>();
	    for(const entt::entity e: view)
	    {
	        const ComponentTransform3D& ctransform = view.get<ComponentTransform3D>(e);
	        ComponentPBRMaterial& cmaterial = view.get<ComponentPBRMaterial>(e);
	        ComponentMesh& cmesh = view.get<ComponentMesh>(e);
	        if(cmaterial.is_ready() && cmesh.is_ready())
				Renderer3D::draw_mesh(cmesh.vertex_array, ctransform.get_model_matrix(), cmaterial.material, &cmaterial.material_data);
		}
		Renderer3D::end_deferred_pass();
	}
};

} // namespace erwin