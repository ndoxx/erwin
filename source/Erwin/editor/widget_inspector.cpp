#include "editor/widget_inspector.h"
#include "level/scene.h"
#include "editor/font_awesome.h"
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

void InspectorWidget::entity_tab()
{
    if(Scene::selected_entity == k_invalid_entity_id)
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
        erwin::visit_entity(Scene::registry, Scene::selected_entity, [](uint64_t reflected_type, void* data)
        {
            // Don't let special editor components be editable this way
            // TODO: automate this via a component meta data flag
            if(reflected_type == "ComponentEditorDescription"_hs)
                return;
            
            const char* component_name = entt::resolve(reflected_type).prop("name"_hs).value().cast<const char*>();
            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
            if(ImGui::TreeNode(component_name))
            {
                // Basic controls over this component
                ImGui::SameLine(ImGui::GetWindowWidth()-50);
                if(ImGui::Button(ICON_FA_WINDOW_CLOSE))
                {
                    Scene::mark_for_removal(Scene::selected_entity, reflected_type);
                    DLOG("editor",1) << "Removed component " << component_name << " from entity " << (unsigned long)Scene::selected_entity << std::endl;
                    return;
                }

                // Invoke GUI for this component
                invoke(W_METAFUNC_INSPECTOR_GUI, reflected_type, data);
                ImGui::TreePop();
            }
            ImGui::Separator();
        });

        // Interface to add a new component
        // TODO: invalidate current_component on selection change
        const auto& component_names = get_component_names();
        static uint64_t current_component = component_names.begin()->first;
        const std::string& label = component_names.at(current_component);
        ImGui::TextUnformatted("Add a new component");
        if(ImGui::BeginCombo("##combo_add_component", label.c_str(), ImGuiComboFlags_NoArrowButton))
        {
            for(auto&& [reflected, name]: component_names)
            {
                // If entity already has component, do not show in combo
                if(invoke(W_METAFUNC_HAS_COMPONENT, reflected, Scene::registry, Scene::selected_entity).cast<bool>())
                    continue;

                bool is_selected = (current_component == reflected);
                if(ImGui::Selectable(name.c_str(), is_selected))
                    current_component = reflected;
                if(is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();
        if(ImGui::Button("Add"))
        {
            invoke(W_METAFUNC_CREATE_COMPONENT, current_component, Scene::registry, Scene::selected_entity);
            DLOG("editor",1) << "Added " << component_names.at(current_component) << " to entity " << (unsigned long)Scene::selected_entity << std::endl;
        }

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