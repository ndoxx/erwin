#include "system/bounding_box_system.h"
#include "asset/bounding.h"
#include "entity/component_camera.h"
#include "entity/component_mesh.h"
#include "entity/component_transform.h"
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
    // TODO: Maybe avoid tagging entity if camera position is inside OBB
    Ray::CollisionData data;
    scene.registry.view<ComponentOBB>().each([&ray, &data, &scene](auto e, const auto& OBB) {        
        if(ray.collides_OBB(OBB.model_matrix, OBB.extent_m, OBB.uniform_scale, data))
            scene.registry.emplace<RayHitTag>(e, RayHitTag{data.near});
    });

    return true;
}

void BoundingBoxSystem::update(const GameClock&, Scene& scene)
{
    scene.registry.view<ComponentOBB, ComponentTransform3D>().each([](auto /*e*/, auto& OBB, const auto& transform) {
        // TMP: use global transform
        OBB.update(transform.local.get_model_matrix(), transform.local.uniform_scale);
    });

    // TODO: instead of doing this in update, make BoundingBoxSystem respond to
    // a MeshChangedEvent of some sort, or a tag component...
    scene.registry.view<ComponentOBB, ComponentMesh>().each(
        [](auto /*e*/, auto& OBB, const auto& cmesh) { OBB.init(cmesh.mesh.extent); });
}

void BoundingBoxSystem::render(const Scene& scene)
{
    Renderer3D::begin_line_pass();
    scene.registry.view<const ComponentOBB, const SelectedTag>().each([](auto /*e*/, const auto& OBB) {
        Renderer3D::draw_cube(glm::scale(OBB.model_matrix, OBB.scale), {1.f, 0.5f, 0.f});
    });
    // TMP: Fails to compile with exclude and a const view. Cant exclude<const SelectedTag> because
    // component type must equal decay type. Waiting for a future fix.
    // https://github.com/skypjack/entt/issues/507
    scene.registry.view<const ComponentOBB>(/*entt::exclude<SelectedTag>*/).each([](auto /*e*/, const auto& OBB) {
        if(OBB.display) // TODO: || editor_show_OBBs
            Renderer3D::draw_cube(glm::scale(OBB.model_matrix, OBB.scale), {0.f, 0.5f, 1.f});
    });
    Renderer3D::end_line_pass();
}

} // namespace editor