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
    EntityID selected = editor::Scene::selected_entity;
    float nearest = editor::Scene::camera_controller.get_zfar();
    
    Ray::CollisionData data;
    auto view = editor::Scene::registry.view<ComponentOBB>();
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
	   editor::Scene::select(selected);

	return true;
}

void BoundingBoxSystem::update(const GameClock& clock)
{
    auto view = editor::Scene::registry.view<ComponentOBB,ComponentTransform3D>();
    for(const entt::entity e: view)
    {
        const ComponentTransform3D& transform = view.get<ComponentTransform3D>(e);
        ComponentOBB& OBB = view.get<ComponentOBB>(e);
        OBB.update(transform.get_model_matrix(), transform.uniform_scale);
    }
}

void BoundingBoxSystem::render()
{
    if(editor::Scene::selected_entity == k_invalid_entity_id)
        return;

    Renderer3D::begin_line_pass();
    auto view = editor::Scene::registry.view<ComponentOBB>();
    for(const entt::entity e: view)
    {
        const ComponentOBB& OBB = view.get<ComponentOBB>(e);

        if(e == editor::Scene::selected_entity)
            Renderer3D::draw_cube(glm::scale(OBB.model_matrix, glm::vec3(1.001f)), {1.f,0.5f,0.f});
        else if(OBB.display) // TODO: || editor_show_OBBs
            Renderer3D::draw_cube(glm::scale(OBB.model_matrix, glm::vec3(1.001f)), {0.f,0.5f,1.f});
    }
    Renderer3D::end_line_pass();
}

} // namespace erwin