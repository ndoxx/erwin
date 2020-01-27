#include "game/bounding_box_system.h"
#include "asset/bounding.h"

namespace erwin
{

BoundingBoxSystem::BoundingBoxSystem(EntityManager* manager): BaseType(manager)
{
    EVENTBUS.subscribe(this, &BoundingBoxSystem::on_ray_scene_query_event);
}

bool BoundingBoxSystem::on_ray_scene_query_event(const RaySceneQueryEvent& event)
{
    auto& scene = Application::SCENE();

    glm::mat4 VP_inv = glm::inverse(scene.camera_controller.get_camera().get_view_projection_matrix());
    Ray ray(event.coords, VP_inv);

    // Perform a ray scene query
    EntityID selected = scene.entities[scene.selected_entity_idx].id;
    float nearest = scene.camera_controller.get_zfar();
    
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
	scene.select(selected);

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

} // namespace erwin