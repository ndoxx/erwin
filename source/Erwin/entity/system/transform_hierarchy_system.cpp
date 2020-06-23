#include "entity/system/transform_hierarchy_system.h"
#include "entity/component/hierarchy.h"
#include "entity/component/transform.h"
#include "core/game_clock.h"

namespace erwin
{

void TransformSystem::update(const GameClock& /*clock*/, entt::registry& registry)
{
	// OPT: only touch the global transforms of entities for which the local transform has changed
	// -> Use a DirtyTransform tag component to signal this
	// see: https://skypjack.github.io/2019-08-20-ecs-baf-part-4-insights/
    registry.view<HierarchyComponent, ComponentTransform3D>().each([&registry](auto /*ent*/, const auto& hier, auto& transform) {
        transform.global = transform.local;
        if(hier.parent != k_invalid_entity_id)
            transform.global += registry.get<ComponentTransform3D>(hier.parent).global;
    });

    // Update global transform of entities that do not have a hierarchy component
    registry.view<ComponentTransform3D>(entt::exclude<HierarchyComponent>).each([](auto /*ent*/, auto& transform) {
        transform.global = transform.local;
    });
}

} // namespace erwin