#include "entity/component_transform.h"

namespace erwin
{

POOL_DEFINITION(ComponentTransform2D);
bool ComponentTransform2D::init(void* description)
{

	return true;
}

POOL_DEFINITION(ComponentTransform3D);
bool ComponentTransform3D::init(void* description)
{

	return true;
}

} // namespace erwin