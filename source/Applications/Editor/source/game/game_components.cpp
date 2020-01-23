#include "game/game_components.h"

namespace erwin
{

COMPONENT_DEFINITION(ComponentRenderablePBRDeferred);
ComponentRenderablePBRDeferred::ComponentRenderablePBRDeferred()
{
	material.data = &material_data;
	material.data_size = sizeof(MaterialData);
}
bool ComponentRenderablePBRDeferred::init(void* description)
{

	return true;
}


COMPONENT_DEFINITION(ComponentRenderableDirectionalLight);
ComponentRenderableDirectionalLight::ComponentRenderableDirectionalLight()
{
	material.data = &material_data;
	material.data_size = sizeof(MaterialData);
}
bool ComponentRenderableDirectionalLight::init(void* description)
{

	return true;
}

} // namespace erwin