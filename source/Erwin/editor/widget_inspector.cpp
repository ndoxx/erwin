#include "editor/widget_inspector.h"
#include "editor/scene.h"
#include "editor/editor_components.h"
#include "entity/reflection.h"
#include "render/renderer_pp.h"
#include "imgui.h"

using namespace erwin;

namespace editor
{

InspectorWidget::InspectorWidget():
Widget("Inspector", true)
{

}

InspectorWidget::~InspectorWidget()
{

}

void InspectorWidget::entity_tab()
{
    if(editor::Scene::selected_entity == k_invalid_entity_id)
        return;
    
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("Properties"))
    {
        ImGui::Text("EntityID: %u", Scene::selected_entity);

        // TODO: Use a resize callback to wire the InputText to the string directly
        auto& desc = Scene::registry.get<ComponentEditorDescription>(Scene::selected_entity);
        static char name_buf[128] = "";
        if(ImGui::InputTextWithHint("Name", "rename entity", name_buf, IM_ARRAYSIZE(name_buf)))
            desc.name = name_buf;

        ImGui::TreePop();
        ImGui::Separator();
    }

    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("Components"))
    {
        erwin::visit_entity(Scene::registry, Scene::selected_entity, W_METAFUNC_INSPECTOR_GUI);

        /*ECS::inspector_GUI(Scene::selected_entity);
        for(auto&& [cid, pcmp]: entity.get_components())
        {
            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
            if(ImGui::TreeNode(pcmp->get_debug_name().c_str()))
            {
                pcmp->inspector_GUI();
                ImGui::TreePop();
            }
            ImGui::Separator();
        }*/

        ImGui::TreePop();
    }
}

void InspectorWidget::on_imgui_render()
{
	static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_Reorderable;

	if(ImGui::BeginTabBar("InspectorTabs", tab_bar_flags))
	{
		if(ImGui::BeginTabItem("Entity"))
		{
			entity_tab();
			ImGui::EndTabItem();
		}
		if(ImGui::BeginTabItem("Post-Processing"))
		{
			PostProcessingRenderer::on_imgui_render();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}


} // namespace editor