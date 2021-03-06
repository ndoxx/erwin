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
    if(!scene.is_loaded())
        return;

    const ComponentCamera3D& camera = scene.get_component<ComponentCamera3D>(scene.get_named("Camera"_h));
    float nearest = camera.frustum.far;
    EntityID selected = k_invalid_entity_id;

    scene.view<RayHitTag>()
        .each([&nearest, &selected](auto e, const auto& data) {
            if(data.near < nearest)
            {
                nearest = data.near;
                selected = e;
            }
        });

    if(selected != k_invalid_entity_id)
    {
        scene.clear<SelectedTag>();
        scene.add_component<SelectedTag>(selected);
    }

    // Clear tags
    scene.clear<RayHitTag>();
}

} // namespace editor