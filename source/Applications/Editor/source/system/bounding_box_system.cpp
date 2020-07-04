#include "system/bounding_box_system.h"
#include "asset/bounding.h"
#include "entity/component/camera.h"
#include "entity/component/mesh.h"
#include "entity/component/tags.h"
#include "entity/component/transform.h"
#include "entity/tag_components.h"
#include "level/scene_manager.h"

using namespace erwin;

namespace editor
{

bool BoundingBoxSystem::on_ray_scene_query_event(const RaySceneQueryEvent& event)
{
    auto& scene = scn::current();
    if(!scene.is_loaded())
        return false;

    const ComponentCamera3D& camera = scene.registry.get<ComponentCamera3D>(scene.get_named("Camera"_h));

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

void BoundingBoxSystem::update(const GameClock&, entt::registry& registry)
{
    registry.view<DirtyOBBTag, ComponentOBB, ComponentMesh>().each(
        [](auto /*e*/, auto& OBB, const auto& cmesh) { OBB.init(cmesh.mesh.extent); });

    registry.view<DirtyOBBTag, ComponentOBB, ComponentTransform3D>().each(
        [](auto /*e*/, auto& OBB, const auto& transform) {
            OBB.update(transform.global.get_model_matrix(), transform.global.uniform_scale);
        });

    registry.clear<DirtyOBBTag>();
}

void BoundingBoxSystem::render(const entt::registry& registry)
{
    Renderer3D::begin_line_pass();
    registry.view<const ComponentOBB, const SelectedTag>().each([](auto /*e*/, const auto& OBB) {
        Renderer3D::draw_cube(glm::scale(OBB.model_matrix, OBB.scale), {1.f, 0.5f, 0.f});
    });
    // TMP: Fails to compile with exclude and a const view. Cant exclude<const SelectedTag> because
    // component type must equal decay type. Waiting for a future fix.
    // https://github.com/skypjack/entt/issues/507
    registry.view<const ComponentOBB>(/*entt::exclude<SelectedTag>*/).each([](auto /*e*/, const auto& OBB) {
        if(OBB.display) // TODO: || editor_show_OBBs
            Renderer3D::draw_cube(glm::scale(OBB.model_matrix, OBB.scale), {0.f, 0.5f, 1.f});
    });
    Renderer3D::end_line_pass();
}

} // namespace editor