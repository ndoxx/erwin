#include "entity/component_mesh.h"
#include "entity/reflection.h"
#include "render/common_geometry.h"
#include "asset/asset_manager.h"
#include "widget/dialog_open.h"
#include "project/project.h"
#include "imgui.h"

namespace erwin
{

template <>
void inspector_GUI<ComponentMesh>(ComponentMesh* cmp)
{
    // Load mesh from file
    if(ImGui::Button("Load"))
        editor::dialog::show_open("ChooseWeshDlgKey", "Choose mesh file", ".wesh", editor::project::get_asset_path(editor::project::DirKey::MESH));

    editor::dialog::on_open("ChooseWeshDlgKey", [&cmp](const fs::path& filepath)
    {
        const auto& mesh = AssetManager::load_mesh(filepath);
        // Copy data
        cmp->init(mesh);
    });
    
    // Select mesh from pre-built primitives
    ImGui::SameLine();
    if(ImGui::Button("Primitive"))
        ImGui::OpenPopup("popup_select_mesh");

    if(ImGui::BeginPopup("popup_select_mesh"))
    {
        CommonGeometry::visit_meshes([&cmp](const Mesh& mesh, const std::string& name)
        {
            // Skip mesh if not compatible with shader, allow any mesh if material is not set
            //if(!cmp->is_material_ready() || Renderer3D::is_compatible(mesh.layout, cmp->material))
            {
                if(ImGui::Selectable(name.c_str()))
                {
                    cmp->init(mesh);
                    return true;
                }
            }
            return false;
        });
        ImGui::EndPopup();
    }
}

} // namespace erwin