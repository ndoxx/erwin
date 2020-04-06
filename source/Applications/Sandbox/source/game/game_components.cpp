#include "game/game_components.h"
#include "asset/asset_manager.h"

namespace erwin
{

template <>
void inspector_GUI<ComponentRenderablePBR>(void* data)
{
    ComponentRenderablePBR* cmp = static_cast<ComponentRenderablePBR*>(data);

    if(ImGui::BeginCombo("##combo_material", "Change material", ImGuiComboFlags_NoArrowButton))
    {
        // TODO: filter out incompatible materials
        AssetManager::visit_materials([&cmp](MaterialHandle handle, const std::string& name, const std::string& description)
        {
            if(ImGui::Selectable(name.c_str(), false))
            {
                cmp->material = handle;
                return true;
            }

            return false;
        });
        ImGui::EndCombo();
    }

    ImGui::ColorEdit3("Tint", (float*)&cmp->material_data.tint);

    bool enable_emissivity = cmp->is_emissive();
    if(ImGui::Checkbox("Emissive", &enable_emissivity))
    {
        if(enable_emissivity)
            cmp->set_emissive(1.f);
        else
            cmp->disable_emissivity();
    }

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