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
void ComponentRenderablePBRDeferred::inspector_GUI()
{
    ImGui::TextColored({0.f,0.75f,1.f,1.f}, "ComponentRenderablePBRDeferred");
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
    ImGui::TextColored({0.f,0.75f,1.f,1.f}, "ComponentRenderableDirectionalLight");
    ImGui::SliderFloat("App. diameter", &material_data.scale, 0.1f, 0.4f);
}

} // namespace erwin