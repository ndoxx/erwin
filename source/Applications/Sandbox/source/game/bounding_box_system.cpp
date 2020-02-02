#include "game/bounding_box_system.h"
#include "asset/bounding.h"
#include "editor/scene.h"

namespace erwin
{

BoundingBoxSystem::BoundingBoxSystem()
{
    EVENTBUS.subscribe(this, &BoundingBoxSystem::on_ray_scene_query_event);
}

bool BoundingBoxSystem::on_ray_scene_query_event(const RaySceneQueryEvent& event)
{
    glm::mat4 VP_inv = glm::inverse(editor::Scene::camera_controller.get_camera().get_view_projection_matrix());
    Ray ray(event.coords, VP_inv);

    // Perform a ray scene query
    EntityID selected = editor::Scene::entities[editor::Scene::selected_entity_idx].id;
    float nearest = editor::Scene::camera_controller.get_zfar();
    
    Ray::CollisionData data;
	for(auto&& cmp_tuple: components_)
	{
		ComponentOBB* pOBB = eastl::get<ComponentOBB*>(cmp_tuple);
        if(ray.collides_OBB(pOBB->model_matrix, pOBB->extent_m, pOBB->scale, data))
        {
            if(data.near < nearest)
            {
                nearest = data.near;
                selected = pOBB->get_parent_entity();
            }
        }
	}
	editor::Scene::select(selected);

	return true;
}

void BoundingBoxSystem::update(const GameClock& clock)
{
	for(auto&& cmp_tuple: components_)
	{
		ComponentTransform3D* transform = eastl::get<ComponentTransform3D*>(cmp_tuple);
		ComponentOBB* OBB = eastl::get<ComponentOBB*>(cmp_tuple);
		OBB->update(transform->get_model_matrix(), transform->uniform_scale);
	}
}

void BoundingBoxSystem::render()
{
    Renderer3D::begin_line_pass();
    for(auto&& cmp_tuple: components_)
    {
        ComponentOBB* OBB = eastl::get<ComponentOBB*>(cmp_tuple);
        if(OBB->get_parent_entity() == editor::Scene::get_selected_entity())
            Renderer3D::draw_cube(glm::scale(OBB->model_matrix, glm::vec3(1.001f)), {1.f,0.5f,0.f});
        else if(OBB->display) // TODO: || editor_show_OBBs
            Renderer3D::draw_cube(glm::scale(OBB->model_matrix, glm::vec3(1.001f)), {0.f,0.5f,1.f});
    }
    Renderer3D::end_line_pass();
}

} // namespace erwin