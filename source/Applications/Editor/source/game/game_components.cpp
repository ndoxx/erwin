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
void ComponentRenderableDirectionalLight::inspector_GUI()
{
    ImGui::TextColored({0.f,0.75f,1.f,1.f}, "ComponentRenderableDirectionalLight");
    ImGui::SliderFloat("App. diameter", &material_data.scale, 0.1f, 0.4f);
}

} // namespace erwin