#include "system/selection_system.h"
#include "asset/bounding.h"
#include "entity/component_camera.h"
#include "entity/component_mesh.h"
#include "entity/component_transform.h"
#include "entity/tag_components.h"
#include "level/scene.h"

using namespace erwin;

namespace editor
{

void SelectionSystem::update(const GameClock&, Scene& scene)
{
    const ComponentCamera3D& camera = scene.registry.get<ComponentCamera3D>(scene.camera);
    float nearest = camera.frustum.far;
    EntityID selected = k_invalid_entity_id;

    // Check gizmo handles selection first, then other entities
    // BUG: When (for example) camera is selected via hierarchy widget,
    // gizmo OBBs are still around previously selected object and
    // will respond to ray hit first, possibly blocking a legit entity selection query.
    scene.registry.view<RayHitTag, GizmoHandleComponent>().each(
        [&nearest, &selected](auto e, const auto& data, const auto& /*gh*/) {
            if(data.near < nearest)
            {
                nearest = data.near;
                selected = e;
            }
        });

    if(selected != k_invalid_entity_id)
    {
        scene.registry.clear<RayHitTag, GizmoHandleSelectedTag>();
        scene.registry.emplace<GizmoHandleSelectedTag>(selected);
        return;
    }

    scene.registry.view<RayHitTag>(entt::exclude<GizmoHandleComponent>)
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
        scene.registry.clear<GizmoHandleSelectedTag, SelectedTag>();
        scene.registry.emplace<SelectedTag>(selected);
    }

    // Clear tags
    scene.registry.clear<RayHitTag>();
}

} // namespace editor