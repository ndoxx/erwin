#include "system/bounding_box_system.h"
#include "asset/bounding.h"
#include "level/scene.h"

namespace erwin
{

bool BoundingBoxSystem::on_ray_scene_query_event(const RaySceneQueryEvent& event)
{
    const ComponentCamera3D& camera = Scene::registry.get<ComponentCamera3D>(Scene::camera);

    glm::mat4 VP_inv = glm::inverse(camera.view_projection_matrix);
    Ray ray(event.coords, VP_inv);

    // Perform a ray scene query
    EntityID selected = Scene::selected_entity;
    float nearest = camera.frustum.far;
    
    Ray::CollisionData data;
    auto view = Scene::registry.view<ComponentOBB>();
    for(const entt::entity e: view)
    {
        const ComponentOBB& OBB = view.get<ComponentOBB>(e);
        if(ray.collides_OBB(OBB.model_matrix, OBB.extent_m, OBB.scale, data))
        {
            if(data.near < nearest)
            {
                nearest = data.near;
                selected = e;
            }
        }
    }

    if(selected != k_invalid_entity_id)
	   Scene::select(selected);

	return true;
}

void BoundingBoxSystem::update(const GameClock&)
{
    auto view = Scene::registry.view<ComponentOBB,ComponentTransform3D>();
    for(const entt::entity e: view)
    {
        const ComponentTransform3D& transform = view.get<ComponentTransform3D>(e);
        ComponentOBB& OBB = view.get<ComponentOBB>(e);
        OBB.update(transform.get_model_matrix(), transform.uniform_scale);
    }
}

void BoundingBoxSystem::render()
{
    if(Scene::selected_entity == k_invalid_entity_id)
        return;

    Renderer3D::begin_line_pass();
    auto view = Scene::registry.view<ComponentOBB>();
    for(const entt::entity e: view)
    {
        const ComponentOBB& OBB = view.get<ComponentOBB>(e);

        if(e == Scene::selected_entity)
            Renderer3D::draw_cube(glm::scale(OBB.model_matrix, glm::vec3(1.001f)), {1.f,0.5f,0.f});
        else if(OBB.display) // TODO: || editor_show_OBBs
            Renderer3D::draw_cube(glm::scale(OBB.model_matrix, glm::vec3(1.001f)), {0.f,0.5f,1.f});
    }
    Renderer3D::end_line_pass();
}

} // namespace erwin