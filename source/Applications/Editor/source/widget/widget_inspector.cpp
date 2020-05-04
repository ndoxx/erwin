#include "widget/widget_inspector.h"
#include "level/scene.h"
#include "imgui/font_awesome.h"
#include "entity/reflection.h"
#include "entity/component_description.h"
#include "render/renderer_pp.h"
#include "render/renderer_3d.h"
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
        auto& desc = Scene::registry.get<ComponentDescription>(Scene::selected_entity);
        static char name_buf[128] = "";
        if(ImGui::InputTextWithHint("Name", "rename entity", name_buf, IM_ARRAYSIZE(name_buf)))
            desc.name = name_buf;

        ImGui::TreePop();
        ImGui::Separator();
    }

    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("Components"))
    {
        erwin::visit_entity(Scene::registry, Scene::selected_entity, [](uint32_t reflected_type, void* data)
        {
            // Don't let special editor components be editable this way
            // TODO: automate this via a component meta data flag
            if(reflected_type == "ComponentDescription"_hs)
                return;
            
            const char* component_name = entt::resolve(reflected_type).prop("name"_hs).value().cast<const char*>();
            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
            if(ImGui::TreeNode(component_name))
            {
                // Basic controls over this component
                ImGui::SameLine(ImGui::GetWindowWidth()-50);
                if(ImGui::Button(W_ICON(WINDOW_CLOSE)))
                {
                    Scene::mark_for_removal(Scene::selected_entity, reflected_type);
                    DLOG("editor",1) << "Removed component " << component_name << " from entity " << static_cast<unsigned long>(Scene::selected_entity) << std::endl;
                    return;
                }

                // Invoke GUI for this component
                invoke(W_METAFUNC_INSPECTOR_GUI, reflected_type, data);
                ImGui::TreePop();
            }
            ImGui::Separator();
        });

        // Interface to add a new component
        if(ImGui::Button("Add component"))
            ImGui::OpenPopup("popup_select_component");

        if(ImGui::BeginPopup("popup_select_component"))
        {
            const auto& component_names = get_component_names();
            for(auto&& [reflected, name]: component_names)
            {
                // If entity already has component, do not show in combo
                if(invoke(W_METAFUNC_HAS_COMPONENT, reflected, Scene::registry, Scene::selected_entity).cast<bool>())
                    continue;

                if(ImGui::Selectable(name.c_str()))
                {
                    invoke(W_METAFUNC_CREATE_COMPONENT, reflected, Scene::registry, Scene::selected_entity);
                    DLOG("editor",1) << "Added " << component_names.at(reflected) 
                                     << " to entity " << static_cast<unsigned long>(Scene::selected_entity) << std::endl;
                    break;
                }
            }
            ImGui::EndPopup();
        }

        ImGui::TreePop();
    }
}

static bool s_enable_IBL = true;
static float s_ambient_strength = 0.15f;
void InspectorWidget::environment_tab()
{
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("IBL"))
    {
        if(ImGui::Checkbox("IBL", &s_enable_IBL))
        {
            Renderer3D::enable_IBL(s_enable_IBL);
        }

        if(s_enable_IBL)
        {
            if(ImGui::SliderFloat("Ambient str.", &s_ambient_strength, 0.f, 1.f))
            {
                Renderer3D::set_IBL_ambient_strength(s_ambient_strength);
            }
        }

        ImGui::TreePop();
    }
}

void InspectorWidget::on_imgui_render()
{
	if(ImGui::BeginTabBar("InspectorTabs", ImGuiTabBarFlags_Reorderable))
	{
		if(ImGui::BeginTabItem("Entity"))
		{
			entity_tab();
			ImGui::EndTabItem();
		}
        if(ImGui::BeginTabItem("Environment"))
        {
            environment_tab();
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