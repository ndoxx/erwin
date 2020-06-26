#include "entity/init.h"
#include "entity/reflection.h"
#include "entity/component/transform.h"
#include "entity/component/camera.h"
#include "entity/component/bounding_box.h"
#include "entity/component/mesh.h"
#include "entity/component/PBR_material.h"
#include "entity/component/dirlight_material.h"
#include "entity/component/light.h"

namespace erwin
{
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
}

} // namespace entity
} // namespace erwin