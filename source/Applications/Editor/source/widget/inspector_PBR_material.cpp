#include "entity/component/PBR_material.h"
#include "entity/reflection.h"
#include "asset/asset_manager.h"
#include "filesystem/filesystem.h"
#include "project/project.h"
#include "widget/dialog_open.h"
#include "imgui/font_awesome.h"
#include "imgui/imgui_utils.h"
#include "imgui.h"

namespace erwin
{

template <>
void inspector_GUI<ComponentPBRMaterial>(ComponentPBRMaterial& cmp, EntityID, entt::registry&, size_t asset_registry)
{
    // Load material from file
    if(ImGui::Button("Load"))
        editor::dialog::show_open("ChooseTomDlgKey", "Choose material file", ".tom", editor::project::asset_dir(editor::DK::MATERIAL).absolute());

    editor::dialog::on_open("ChooseTomDlgKey", [&cmp,asset_registry](const fs::path& filepath)
    {
        cmp = AssetManager::load<ComponentPBRMaterial>(asset_registry, WPath(filepath)); // // Copy material
    });
    
    // ImGui::TextUnformatted(cmp.name.c_str()); // Name unused atm

    // Parameters override
    ImGui::Checkbox("Override", &cmp.override);

    if(cmp.override)
    {
        // PBR parameters
        ImGui::ColorEdit3("Tint", static_cast<float*>(&cmp.material_data.tint[0]));
        ImGui::SliderFloatDefault("Tiling", &cmp.material_data.tiling_factor, 0.1f, 10.f, 1.f);

        bool enable_emissivity = cmp.is_emissive();
        if(ImGui::Checkbox("Emissive", &enable_emissivity))
            cmp.enable_emissivity(enable_emissivity);
        if(cmp.is_emissive())
        {
            ImGui::SameLine();
            ImGui::SliderFloatDefault("##Emissivity", &cmp.material_data.emissive_scale, 0.1f, 10.f, 1.f);
        }

        bool enable_parallax = cmp.has_parallax();
        if(ImGui::Checkbox("Parallax", &enable_parallax))
            cmp.enable_parallax(enable_parallax);
        if(cmp.has_parallax())
        {
            ImGui::SameLine();
            ImGui::SliderFloatDefault("##Parallax", &cmp.material_data.parallax_height_scale, 0.005f, 0.05f, 0.03f);
        }

        // Uniform parameters
        ImGui::ColorEdit3("Unif. albedo", static_cast<float*>(&cmp.material_data.uniform_albedo[0]));
        ImGui::SliderFloat("Unif. metal", &cmp.material_data.uniform_metallic, 0.f, 1.f);
        ImGui::SliderFloat("Unif. rough", &cmp.material_data.uniform_roughness, 0.f, 1.f);
    }
}

} // namespace erwin