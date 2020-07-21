#include "entity/component/mesh.h"
#include "entity/component/tags.h"
#include "entity/reflection.h"
#include "render/common_geometry.h"
#include "asset/asset_manager.h"
#include "widget/dialog_open.h"
#include "project/project.h"
#include "imgui.h"

namespace erwin
{

template <>
void inspector_GUI<ComponentMesh>(ComponentMesh& cmp, EntityID e, entt::registry& registry, size_t asset_registry)
{
    // Load mesh from file
    if(ImGui::Button("Load"))
        editor::dialog::show_open("ChooseWeshDlgKey", "Choose mesh file", ".wesh", editor::project::asset_dir(editor::DK::MESH).absolute());

    editor::dialog::on_open("ChooseWeshDlgKey", [&cmp,&registry,e,asset_registry](const fs::path& filepath)
    {
        cmp.mesh = AssetManager::load<Mesh>(asset_registry, WPath("res", filepath));
        registry.emplace_or_replace<DirtyOBBTag>(e);
    });
    
    // Select mesh from pre-built primitives
    ImGui::SameLine();
    if(ImGui::Button("Primitive"))
        ImGui::OpenPopup("popup_select_mesh");

    if(ImGui::BeginPopup("popup_select_mesh"))
    {
        CommonGeometry::visit_meshes([&cmp,&registry,e](const Mesh& mesh, const std::string& name)
        {
            if(ImGui::Selectable(name.c_str()))
            {
                cmp.mesh = mesh;
                registry.emplace_or_replace<DirtyOBBTag>(e);
                return true;
            }
            return false;
        });
        ImGui::EndPopup();
    }
}

} // namespace erwin