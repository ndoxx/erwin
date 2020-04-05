#include "game/game_components.h"

namespace erwin
{

template <>
void inspector_GUI<ComponentRenderablePBR>(void* data)
{
    ComponentRenderablePBR* cmp = static_cast<ComponentRenderablePBR*>(data);

    ImGui::ColorEdit3("Tint", (float*)&cmp->material_data.tint);

    if(cmp->is_emissive())
    	ImGui::SliderFloat("Emissivity", &cmp->material_data.emissive_scale, 0.1f, 10.0f);
}

template <>
void inspector_GUI<ComponentRenderableDirectionalLight>(void* data)
{
    ComponentRenderableDirectionalLight* cmp = static_cast<ComponentRenderableDirectionalLight*>(data);

    ImGui::SliderFloat("App. diameter", &cmp->material_data.scale, 0.1f, 0.4f);
}

} // namespace erwin