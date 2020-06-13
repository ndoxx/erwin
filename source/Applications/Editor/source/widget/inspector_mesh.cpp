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
    // Select mesh from pre-built primitives
    if(ImGui::Button("Primitive"))
        ImGui::OpenPopup("popup_select_mesh");

    if(ImGui::BeginPopup("popup_select_mesh"))
    {
        CommonGeometry::visit_meshes([&cmp](const Mesh& mesh)
        {
            // Skip mesh if not compatible with shader, allow any mesh if material is not set
            //if(!cmp->is_material_ready() || Renderer3D::is_compatible(mesh.layout, cmp->material))
            {
                if(ImGui::Selectable(mesh.name))
                {
                    cmp->set_vertex_array(mesh.VAO);
                    cmp->extent = mesh.extent;
                    // TODO: Update OBB if any (send event?)
                    return true;
                }
            }
            return false;
        });
        ImGui::EndPopup();
    }

    // Load mesh from file
    ImGui::SameLine();
    if(ImGui::Button("Load"))
        editor::dialog::show_open("ChooseWeshDlgKey", "Choose mesh file", ".wesh", editor::project::get_asset_path(editor::project::DirKey::MESH));

    editor::dialog::on_open("ChooseWeshDlgKey", [&cmp](const fs::path& filepath)
    {
        const auto& mesh = AssetManager::load_mesh(filepath);
        // Copy data
        cmp->set_vertex_array(mesh.VAO);
        cmp->extent = mesh.extent;
    });
}

} // namespace erwin