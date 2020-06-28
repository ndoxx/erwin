#include "entity/system/transform_hierarchy_system.h"
#include "core/game_clock.h"
#include "entity/component/hierarchy.h"
#include "entity/component/tags.h"
#include "entity/component/transform.h"

namespace erwin
{

void TransformSystem::update(const GameClock& /*clock*/, entt::registry& registry)
{
    // Update the whole subtree of entities for which the local transform has changed
    // OPT: Still room for improvement: multiple members of the same subtree may be updated during
    // the same frame and marked dirty, but it is useless to perform subtree traversal for each of them.
    // OPT: Breadth-first traversal should be cache-friendlier considering the pool ordering, however
    // my tests with cachegrind aren't conclusive. 
    registry.view<ComponentHierarchy, ComponentTransform3D, DirtyTransformTag>().each(
        [&registry](auto e, const auto&, const auto&) {
            entity::depth_first(e, registry, [&registry](EntityID child, const auto& child_hier, size_t) {
                auto& child_transform = registry.get<ComponentTransform3D>(child);
                child_transform.global =
                    (child_hier.parent != k_invalid_entity_id)
                        ? Transform3D::compose(child_transform.local,
                                               registry.get<ComponentTransform3D>(child_hier.parent).global)
                        : child_transform.local;
                registry.emplace_or_replace<DirtyOBBTag>(child);
                return false;
            });
        });

    registry.clear<DirtyTransformTag>();

    // Update global transform of entities that do not have a hierarchy component
    registry.view<ComponentTransform3D>(entt::exclude<ComponentHierarchy>).each([](auto /*ent*/, auto& transform) {
        transform.global = transform.local;
    });
}

} // namespace erwin