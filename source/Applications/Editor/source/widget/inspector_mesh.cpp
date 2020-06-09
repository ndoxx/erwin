#include "entity/component_mesh.h"
#include "entity/reflection.h"
#include "render/common_geometry.h"
#include "imgui.h"

namespace erwin
{

template <>
void inspector_GUI<ComponentMesh>(ComponentMesh* cmp)
{
    // Select mesh
    // TMP: Only common geometry meshes for now
    if(ImGui::Button("Mesh"))
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
                    // TODO: Update OBB if any (send event?)
                    return true;
                }
            }
            return false;
        });
        ImGui::EndPopup();
    }
}

} // namespace erwin