#include "entity/init.h"
#include "entity/reflection.h"
#include "entity/component/serial/transform.h"
#include "entity/component/serial/camera.h"
#include "entity/component/serial/bounding_box.h"
#include "entity/component/serial/mesh.h"
#include "entity/component/serial/PBR_material.h"
#include "entity/component/serial/dirlight_material.h"
#include "entity/component/serial/light.h"
#include "entity/component/serial/description.h"

namespace erwin
{

// Defined in the editor
// TODO: Check that it still works when client does not define these symbols
template <> void inspector_GUI<ComponentTransform3D>(ComponentTransform3D& cmp, EntityID e, entt::registry& registry, size_t);
template <> void inspector_GUI<ComponentCamera3D>(ComponentCamera3D& cmp, EntityID e, entt::registry& registry, size_t);
template <> void inspector_GUI<ComponentOBB>(ComponentOBB& cmp, EntityID e, entt::registry& registry, size_t);
template <> void inspector_GUI<ComponentMesh>(ComponentMesh& cmp, EntityID e, entt::registry& registry, size_t);
template <> void inspector_GUI<ComponentPBRMaterial>(ComponentPBRMaterial& cmp, EntityID e, entt::registry& registry, size_t);
template <> void inspector_GUI<ComponentDirectionalLightMaterial>(ComponentDirectionalLightMaterial& cmp, EntityID e, entt::registry& registry, size_t);
template <> void inspector_GUI<ComponentDirectionalLight>(ComponentDirectionalLight& cmp, EntityID e, entt::registry& registry, size_t);

namespace entity
{

void init_components()
{
    REFLECT_COMPONENT(ComponentTransform3D);
    REFLECT_COMPONENT(ComponentCamera3D);
    REFLECT_COMPONENT(ComponentOBB);
    REFLECT_COMPONENT(ComponentMesh);
    REFLECT_COMPONENT(ComponentPBRMaterial);
    REFLECT_COMPONENT(ComponentDirectionalLightMaterial);
    REFLECT_COMPONENT(ComponentDirectionalLight);

    REFLECT_COMPONENT(ComponentDescription);
    HIDE_FROM_INSPECTOR(ComponentDescription);
}

} // namespace entity
} // namespace erwin