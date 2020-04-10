#include "game/game_components.h"
#include "asset/asset_manager.h"
#include "editor/font_awesome.h"
#include "render/common_geometry.h"
#include "render/renderer.h"
#include "imgui/imgui_utils.h"

namespace erwin
{

template <>
void inspector_GUI<ComponentRenderablePBR>(ComponentRenderablePBR* cmp)
{
    // Select material
    if(ImGui::Button("Material"))
        ImGui::OpenPopup("popup_select_material");
    
    ImGui::SameLine();
    if(cmp->material.is_valid())
        ImGui::TextUnformatted(AssetManager::get_name(cmp->material).c_str());
    else
        ImGui::TextUnformatted("None");

    if(ImGui::BeginPopup("popup_select_material"))
    {
        AssetManager::visit_materials([&cmp](MaterialHandle handle, const std::string& name, const std::string& description)
        {
            if(ImGui::Selectable(name.c_str()))
            {
                cmp->set_material(handle);
                return true;
            }
            return false;
        });
        ImGui::EndPopup();
    }


    // Select mesh
    // TMP: Only common geometry meshes for now
    if(ImGui::Button("Mesh"))
        ImGui::OpenPopup("popup_select_mesh");

    if(ImGui::BeginPopup("popup_select_mesh"))
    {
        CommonGeometry::visit_meshes([&cmp](const MeshStub& mesh)
        {
            // Skip mesh if not compatible with shader, allow any mesh if material is not set
            if(!cmp->material.is_valid() || Renderer3D::is_compatible(mesh.layout, cmp->material))
            {
                if(ImGui::Selectable(mesh.name))
                {
                    cmp->set_vertex_array(mesh.VAO);
                    // TODO: Update OBB if any (send event?)
                    return true;
                }
            }
            return false;
        });
        ImGui::EndPopup();
    }


    // PBR parameters
    ImGui::ColorEdit3("Tint", (float*)&cmp->material_data.tint);
    ImGui::SliderFloatDefault("Tiling", &cmp->material_data.tiling_factor, 0.1f, 10.f, 1.f);

    bool enable_emissivity = cmp->is_emissive();
    if(ImGui::Checkbox("Emissive", &enable_emissivity))
    {
        if(enable_emissivity)
            cmp->set_emissive(1.f);
        else
            cmp->disable_emissivity();
    }

    if(cmp->is_emissive())
        ImGui::SliderFloatDefault("Emissivity", &cmp->material_data.emissive_scale, 0.1f, 10.f, 1.f);
}

template <>
void inspector_GUI<ComponentRenderableDirectionalLight>(ComponentRenderableDirectionalLight* cmp)
{
    ImGui::SliderFloat("App. diameter", &cmp->material_data.scale, 0.1f, 0.4f);
}

} // namespace erwin