#include "game/game_components.h"

namespace erwin
{

COMPONENT_DEFINITION(ComponentRenderablePBR);
ComponentRenderablePBR::ComponentRenderablePBR()
{
	material.data = &material_data;
	material.data_size = sizeof(MaterialData);
}
bool ComponentRenderablePBR::init(void* description)
{

	return true;
}
void ComponentRenderablePBR::inspector_GUI()
{
    ImGui::ColorEdit3("Tint", (float*)&material_data.tint);

    if(is_emissive())
    	ImGui::SliderFloat("Emissive scale", &material_data.emissive_scale, 0.1f, 10.0f);
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
    ImGui::SliderFloat("App. diameter", &material_data.scale, 0.1f, 0.4f);
}

} // namespace erwin