#include "game/game_components.h"

namespace erwin
{

COMPONENT_DEFINITION(ComponentRenderablePBRDeferred);

ComponentRenderablePBRDeferred::ComponentRenderablePBRDeferred()
{
	material.data = &material_data;
}

bool ComponentRenderablePBRDeferred::init(void* description)
{

	return true;
}

} // namespace erwin