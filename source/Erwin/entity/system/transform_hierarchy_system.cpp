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
        if(hier.parent != k_invalid_entity_id)
        {
        	const auto& parent_transform = registry.get<ComponentTransform3D>(hier.parent);
        	auto pr = glm::mat3(parent_transform.global.get_model_matrix());

        	transform.global.position = pr * (transform.local.uniform_scale * transform.local.position) + parent_transform.global.position;
        	transform.global.rotation = parent_transform.global.rotation * transform.local.rotation;
        	transform.global.euler    = glm::eulerAngles(transform.global.rotation);
        	transform.global.uniform_scale = parent_transform.global.uniform_scale * transform.local.uniform_scale;
        }
        else
        	transform.global = transform.local;
    });

    // Update global transform of entities that do not have a hierarchy component
    registry.view<ComponentTransform3D>(entt::exclude<HierarchyComponent>).each([](auto /*ent*/, auto& transform) {
        transform.global = transform.local;
    });
}

} // namespace erwin