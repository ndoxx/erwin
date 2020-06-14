#include "system/bounding_box_system.h"
#include "asset/bounding.h"
#include "entity/component_camera.h"
#include "entity/component_transform.h"
#include "entity/component_mesh.h"
#include "level/scene.h"

using namespace erwin;

namespace editor
{

bool BoundingBoxSystem::on_ray_scene_query_event(const RaySceneQueryEvent& event)
{
    auto& scene = scn::current<Scene>();
    if(!scene.is_loaded())
        return false;

    const ComponentCamera3D& camera = scene.registry.get<ComponentCamera3D>(scene.camera);

    glm::mat4 VP_inv = glm::inverse(camera.view_projection_matrix);
    Ray ray(event.coords, VP_inv);

    // Perform a ray scene query
    EntityID selected = scene.selected_entity;
    float nearest = camera.frustum.far;

    Ray::CollisionData data;
    auto view = scene.registry.view<ComponentOBB>();
    for(const entt::entity e : view)
    {
        const ComponentOBB& OBB = view.get<ComponentOBB>(e);
        if(ray.collides_OBB(OBB.model_matrix, OBB.extent_m, OBB.uniform_scale, data))
        {
            if(data.near < nearest)
            {
                nearest = data.near;
                selected = e;
            }
        }
    }

    if(selected != k_invalid_entity_id)
        scene.select(selected);

    return true;
}

void BoundingBoxSystem::update(const GameClock&)
{
    auto& scene = scn::current<Scene>();
    if(!scene.is_loaded())
        return;

    {
        auto view = scene.registry.view<ComponentOBB, ComponentTransform3D>();
        for(const entt::entity e : view)
        {
            const ComponentTransform3D& transform = view.get<ComponentTransform3D>(e);
            ComponentOBB& OBB = view.get<ComponentOBB>(e);
            OBB.update(transform.get_model_matrix(), transform.uniform_scale);
        }
    }

    {
        auto view = scene.registry.view<ComponentOBB, ComponentMesh>();
        for(const entt::entity e : view)
        {
            const ComponentMesh& cmesh = view.get<ComponentMesh>(e);
            ComponentOBB& OBB = view.get<ComponentOBB>(e);
            OBB.init(cmesh.mesh.extent);
        }
    }
}

void BoundingBoxSystem::render()
{
    auto& scene = scn::current<Scene>();
    if(!scene.is_loaded())
        return;
    if(scene.selected_entity == k_invalid_entity_id)
        return;

    Renderer3D::begin_line_pass();
    auto view = scene.registry.view<ComponentOBB>();
    for(const entt::entity e : view)
    {
        const ComponentOBB& OBB = view.get<ComponentOBB>(e);

        if(e == scene.selected_entity)
            Renderer3D::draw_cube(glm::scale(OBB.model_matrix, OBB.scale), {1.f, 0.5f, 0.f});
        else if(OBB.display) // TODO: || editor_show_OBBs
            Renderer3D::draw_cube(glm::scale(OBB.model_matrix, OBB.scale), {0.f, 0.5f, 1.f});
    }
    Renderer3D::end_line_pass();
}

} // namespace editor