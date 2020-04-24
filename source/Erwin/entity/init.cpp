#include "entity/init.h"
#include "entity/reflection.h"
#include "entity/component_transform.h"
#include "entity/component_bounding_box.h"
#include "entity/component_mesh.h"
#include "entity/component_PBR_material.h"
#include "entity/component_dirlight_material.h"
#include "entity/light.h"

namespace erwin
{
namespace entity
{


void init_components()
{
    REFLECT_COMPONENT(ComponentTransform3D);
    REFLECT_COMPONENT(ComponentOBB);
    REFLECT_COMPONENT(ComponentMesh);
    REFLECT_COMPONENT(ComponentPBRMaterial);
    REFLECT_COMPONENT(ComponentDirectionalLightMaterial);
    REFLECT_COMPONENT(ComponentDirectionalLight);
}

} // namespace entity
} // namespace erwin