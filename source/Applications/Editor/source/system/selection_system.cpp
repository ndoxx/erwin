#include "system/selection_system.h"
#include "asset/bounding.h"
#include "entity/component_camera.h"
#include "entity/component_transform.h"
#include "entity/component_mesh.h"
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
    {
        auto view = scene.registry.view<RayHitTag, GizmoHandleComponent>();
        for(const entt::entity e : view)
        {
            const auto& data = view.get<RayHitTag>(e);
            if(data.near < nearest)
            {
                nearest = data.near;
                selected = e;
            }
        }

        if(selected != k_invalid_entity_id)
        {
            scene.registry.emplace<GizmoHandleSelectedTag>(selected);
            scene.registry.clear<RayHitTag>();
            return;
        }
    }

    {
        auto view = scene.registry.view<RayHitTag>(entt::exclude<GizmoHandleComponent>);
        for(const entt::entity e : view)
        {
            const auto& data = view.get<RayHitTag>(e);
            if(data.near < nearest)
            {
                nearest = data.near;
                selected = e;
            }
        }

        if(selected != k_invalid_entity_id)
        {
            scene.select(selected);
            // Drop gizmo handle selection
            scene.registry.clear<GizmoHandleSelectedTag>();
        }
    }

    // Clear tags
    scene.registry.clear<RayHitTag>();
}

} // namespace editor