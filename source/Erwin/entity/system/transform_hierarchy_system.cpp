#include "entity/system/transform_hierarchy_system.h"
#include "core/game_clock.h"
#include "entity/component/hierarchy.h"
#include "entity/component/transform.h"

namespace erwin
{

void TransformSystem::update(const GameClock& /*clock*/, entt::registry& registry)
{
    // OPT: only touch the global transforms of entities for which the local transform has changed (also its children)
    // -> Use a DirtyTransform tag component to signal this
    // see: https://skypjack.github.io/2019-08-20-ecs-baf-part-4-insights/
    registry.view<ComponentHierarchy, ComponentTransform3D>().each(
        [&registry](auto /*ent*/, const auto& hier, auto& transform) {
            transform.global = (hier.parent != k_invalid_entity_id)
                ? Transform3D::compose(transform.local, registry.get<ComponentTransform3D>(hier.parent).global)
                : transform.local;
        });

    // Update global transform of entities that do not have a hierarchy component
    registry.view<ComponentTransform3D>(entt::exclude<ComponentHierarchy>).each([](auto /*ent*/, auto& transform) {
        transform.global = transform.local;
    });
}

} // namespace erwin