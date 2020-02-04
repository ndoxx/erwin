#include "editor/widget_inspector.h"
#include "editor/scene.h"
#include "entity/entity_manager.h"
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
        ImGui::Text("EntityID: %lu", Scene::selected_entity);

        // TODO: Use a resize callback to wire the InputText to the string directly
        auto& ent = ECS::get_entity(Scene::selected_entity);
        static char name_buf[128] = "";
        if(ImGui::InputTextWithHint("Name", "rename entity", name_buf, IM_ARRAYSIZE(name_buf)))
            ent.set_name(name_buf);

        ImGui::TreePop();
        ImGui::Separator();
    }

    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("Components"))
    {
        ECS::inspector_GUI(Scene::selected_entity);
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