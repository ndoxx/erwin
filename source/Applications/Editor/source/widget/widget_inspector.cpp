#include "widget/widget_inspector.h"
#include "level/scene.h"
#include "imgui/font_awesome.h"
#include "entity/reflection.h"
#include "entity/component/description.h"
#include "entity/tag_components.h"
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
    auto& scene = scn::current<Scene>();
    if(!scene.is_loaded())
        return;

    auto view = scene.registry.view<SelectedTag, ComponentDescription>(entt::exclude<NonEditableTag>);
    for(const entt::entity e : view)
    {
        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Properties"))
        {
            ImGui::Text("EntityID: %u", e);

            // TODO: Use a resize callback to wire the InputText to the string directly
            ComponentDescription& desc = view.get<ComponentDescription>(e);
            static char name_buf[128] = "";
            if(ImGui::InputTextWithHint("Name", "rename entity", name_buf, IM_ARRAYSIZE(name_buf)))
                desc.name = name_buf;

            ImGui::TreePop();
            ImGui::Separator();
        }

        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Components"))
        {
            erwin::visit_entity(scene.registry, e, [&scene,e](uint32_t reflected_type, void* data)
            {
                // Don't let special editor components be editable this way
                if(is_hidden_from_inspector(reflected_type))
                    return;
                
                const char* component_name = entt::resolve_id(reflected_type).prop("name"_hs).value().cast<const char*>();
                ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
                if(ImGui::TreeNode(component_name))
                {
                    // Basic controls over this component
                    ImGui::SameLine(ImGui::GetWindowWidth()-50);
                    if(ImGui::Button(W_ICON(WINDOW_CLOSE)))
                    {
                        scene.mark_for_removal(e, reflected_type);
                        DLOG("editor",1) << "Removed component " << component_name << " from entity " << static_cast<unsigned long>(e) << std::endl;
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
                    if(invoke(W_METAFUNC_HAS_COMPONENT, reflected, scene.registry, e).cast<bool>())
                        continue;

                    if(ImGui::Selectable(name.c_str()))
                    {
                        invoke(W_METAFUNC_CREATE_COMPONENT, reflected, scene.registry, e);
                        DLOG("editor",1) << "Added " << component_names.at(reflected) 
                                         << " to entity " << static_cast<unsigned long>(e) << std::endl;
                        break;
                    }
                }
                ImGui::EndPopup();
            }

            ImGui::TreePop();
        }

        break; // TMP: Only one entity selectable for now
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