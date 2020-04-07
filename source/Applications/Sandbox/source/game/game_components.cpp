#include "game/game_components.h"
#include "asset/asset_manager.h"
#include "editor/font_awesome.h"
#include "render/common_geometry.h"

namespace erwin
{

template <>
void inspector_GUI<ComponentRenderablePBR>(void* data)
{
    ComponentRenderablePBR* cmp = static_cast<ComponentRenderablePBR*>(data);

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
            // TODO: Skip mesh if not compatible with shader
            if(ImGui::Selectable(mesh.name))
            {
                cmp->set_vertex_array(mesh.VAO);
                // TODO: Update OBB if any (send event?)
                return true;
            }
            return false;
        });
        ImGui::EndPopup();
    }


    // PBR parameters
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