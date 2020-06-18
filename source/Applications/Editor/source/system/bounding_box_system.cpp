#include "system/bounding_box_system.h"
#include "asset/bounding.h"
#include "entity/component_camera.h"
#include "entity/component_transform.h"
#include "entity/component_mesh.h"
#include "entity/tag_components.h"
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

    // Perform a ray scene query, tag all entities whose OBBs are hit by the ray
    Ray::CollisionData data;
    auto view = scene.registry.view<ComponentOBB>();
    for(const entt::entity e : view)
    {
        const ComponentOBB& OBB = view.get<ComponentOBB>(e);
        if(ray.collides_OBB(OBB.model_matrix, OBB.extent_m, OBB.uniform_scale, data))
            scene.registry.emplace<RayHitTag>(e, RayHitTag{data.near});
    }

    return true;
}

void BoundingBoxSystem::update(const GameClock&, Scene& scene)
{
    {
        auto view = scene.registry.view<ComponentOBB, ComponentTransform3D>();
        for(const entt::entity e : view)
        {
            const ComponentTransform3D& transform = view.get<ComponentTransform3D>(e);
            ComponentOBB& OBB = view.get<ComponentOBB>(e);
            OBB.update(transform.get_model_matrix(), transform.uniform_scale);
        }
    }

    // TODO: instead of doing this in update, make BoundingBoxSystem respond to
    // a MeshChangedEvent of some sort, or a tag component...
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

void BoundingBoxSystem::render(const Scene& scene)
{
    Renderer3D::begin_line_pass();
    {
        auto view = scene.registry.view<const SelectedTag, const ComponentOBB>();
        for(const entt::entity e : view)
        {
            const ComponentOBB& OBB = view.get<const ComponentOBB>(e);
            Renderer3D::draw_cube(glm::scale(OBB.model_matrix, OBB.scale), {1.f, 0.5f, 0.f});
        }
    }
    {
        // TMP: Fails to compile with exclude and a const view. Cant exclude<const SelectedTag> because
        // component type must equal decay type. Waiting for a future fix.
        // https://github.com/skypjack/entt/issues/507
        auto view = scene.registry.view<const ComponentOBB>(/*entt::exclude<SelectedTag>*/);
        for(const entt::entity e : view)
        {
            const ComponentOBB& OBB = view.get<const ComponentOBB>(e);
            if(OBB.display) // TODO: || editor_show_OBBs
                Renderer3D::draw_cube(glm::scale(OBB.model_matrix, OBB.scale), {0.f, 0.5f, 1.f});
        }
    }
    Renderer3D::end_line_pass();
}

} // namespace editor