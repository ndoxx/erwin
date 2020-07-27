#include "system/selection_system.h"
#include "asset/bounding.h"
#include "entity/component/camera.h"
#include "entity/component/mesh.h"
#include "entity/component/transform.h"
#include "entity/component/editor_tags.h"
#include "level/scene.h"

using namespace erwin;

namespace editor
{

void SelectionSystem::update(const GameClock&, Scene& scene)
{
    const ComponentCamera3D& camera = scene.get_component<ComponentCamera3D>(scene.get_named("Camera"_h));
    float nearest = camera.frustum.far;
    EntityID selected = k_invalid_entity_id;

    // Check gizmo handles selection first, then other entities
    // BUG #7: When a NoGizmo tagged entity (like camera) is selected via hierarchy widget,
    // gizmo OBBs are still around previously selected object and
    // will respond to ray hit first, shadowing a legit entity selection query.
    /*scene.view<RayHitTag, GizmoHandleComponent>().each(
        [&nearest, &selected](auto e, const auto& data, const auto&) {
            if(data.near < nearest)
            {
                nearest = data.near;
                selected = e;
            }
        });

    if(selected != k_invalid_entity_id)
    {
        scene.clear<RayHitTag, GizmoHandleSelectedTag>();
        scene.add_component<GizmoHandleSelectedTag>(selected);
        return;
    }*/

    scene.view<RayHitTag>(/*entt::exclude<GizmoHandleComponent>*/)
        .each([&nearest, &selected](auto e, const auto& data) {
            if(data.near < nearest)
            {
                nearest = data.near;
                selected = e;
            }
        });

    if(selected != k_invalid_entity_id)
    {
        // Drop gizmo handle selection
        scene.clear</*GizmoHandleSelectedTag, */SelectedTag>();
        scene.add_component<SelectedTag>(selected);
    }

    // Clear tags
    scene.clear<RayHitTag>();
}

} // namespace editor