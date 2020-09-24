#include "system/bounding_box_system.h"
#include "asset/bounding.h"
#include "entity/component/camera.h"
#include "entity/component/mesh.h"
#include "entity/component/tags.h"
#include "entity/component/transform.h"
#include "entity/component/editor_tags.h"
#include "level/scene_manager.h"

#include "glm/gtx/string_cast.hpp"

using namespace erwin;

namespace editor
{

inline bool OBB_contains(const ComponentOBB& OBB, const glm::vec3& point)
{
    auto point_ms = glm::inverse(OBB.model_matrix) * glm::vec4(point,1.f);
    point_ms /= point_ms.w;
    for(int ii=0; ii<3; ++ii)
        if((point_ms[ii] < OBB.extent_m[size_t(2*ii)]) || (point_ms[ii] > OBB.extent_m[size_t(2*ii+1)]))
            return false;
    return true;
}

bool BoundingBoxSystem::on_ray_scene_query_event(const RaySceneQueryEvent& event)
{
    auto& scene = scn::current();
    if(!scene.is_loaded())
        return false;

    auto e_cam = scene.get_named("Camera"_h);
    const ComponentCamera3D& camera = scene.get_component<ComponentCamera3D>(e_cam);
    const ComponentTransform3D& camera_transform = scene.get_component<ComponentTransform3D>(e_cam);

    glm::mat4 VP_inv = glm::inverse(camera.view_projection_matrix);
    Ray ray(event.coords, VP_inv);

    // Perform a ray scene query, tag all entities whose OBBs are hit by the ray
    // Avoid tagging entity if camera position is inside OBB
    Ray::CollisionData data;
    scene.view<ComponentOBB>().each([&ray, &data, &camera_transform, &scene](auto e, const auto& OBB) {
        if(ray.collides_OBB(OBB.model_matrix, OBB.extent_m, OBB.uniform_scale, data))
            if(!OBB_contains(OBB, camera_transform.local.position))
                scene.add_component<RayHitTag>(e, RayHitTag{data.near});
    });

    return true;
}

void BoundingBoxSystem::update(const GameClock&, Scene& scene)
{
    scene.view<DirtyOBBTag, ComponentOBB, ComponentMesh>().each(
        [](auto /*e*/, auto& OBB, const auto& cmesh) { OBB.init(cmesh.mesh.extent); });

    scene.view<DirtyOBBTag, ComponentOBB, ComponentTransform3D>().each(
        [](auto /*e*/, auto& OBB, const auto& transform) {
            OBB.update(transform.global.get_model_matrix(), transform.global.uniform_scale);
        });

    scene.clear<DirtyOBBTag>();
}

void BoundingBoxSystem::render(const Scene& scene)
{
    Renderer3D::begin_line_pass();
    scene.view<const ComponentOBB, const SelectedTag>().each([](auto /*e*/, const auto& OBB) {
        Renderer3D::draw_cube(glm::scale(OBB.model_matrix, OBB.scale), {1.f, 0.5f, 0.f});
    });
    // TMP: Fails to compile with exclude and a const view. Cant exclude<const SelectedTag> because
    // component type must equal decay type. Waiting for a future fix.
    // https://github.com/skypjack/entt/issues/507
    scene.view<const ComponentOBB>(/*entt::exclude<SelectedTag>*/).each([](auto /*e*/, const auto& OBB) {
        if(OBB.display) // TODO: || editor_show_OBBs
            Renderer3D::draw_cube(glm::scale(OBB.model_matrix, OBB.scale), {0.f, 0.5f, 1.f});
    });
    Renderer3D::end_line_pass();
}

} // namespace editor