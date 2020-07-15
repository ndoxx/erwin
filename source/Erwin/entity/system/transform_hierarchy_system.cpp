#include "entity/system/transform_hierarchy_system.h"
#include "core/game_clock.h"
#include "entity/component/hierarchy.h"
#include "entity/component/tags.h"
#include "entity/component/transform.h"
#include "level/scene.h"

namespace erwin
{

void TransformSystem::update(const GameClock& /*clock*/, Scene& scene)
{
    // Update the whole subtree of entities for which the local transform has changed
    // OPT: Still room for improvement: multiple members of the same subtree may be updated during
    // the same frame and marked dirty, but it is useless to perform subtree traversal for each of them.
    // OPT: Breadth-first traversal should be cache-friendlier considering the pool ordering, however
    // my tests with cachegrind aren't conclusive. 
    scene.view<ComponentHierarchy, ComponentTransform3D, DirtyTransformTag>().each(
        [&scene](auto e, const auto&, const auto&) {
            scene.depth_first(e, [&scene](EntityID child, const auto& child_hier, size_t) {
                auto& child_transform = scene.get_component<ComponentTransform3D>(child);
                child_transform.global =
                    (child_hier.parent != k_invalid_entity_id)
                        ? Transform3D::compose(child_transform.local,
                                               scene.get_component<ComponentTransform3D>(child_hier.parent).global)
                        : child_transform.local;
                scene.try_add_component<DirtyOBBTag>(child);
                return false;
            });
        });

    scene.clear<DirtyTransformTag>();

    // Update global transform of entities that do not have a hierarchy component
    scene.view<ComponentTransform3D>(entt::exclude<ComponentHierarchy>).each([](auto /*ent*/, auto& transform) {
        transform.global = transform.local;
    });
}

} // namespace erwin