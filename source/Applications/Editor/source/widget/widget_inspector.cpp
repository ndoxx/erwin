#include "widget/widget_inspector.h"
#include "entity/component/description.h"
#include "entity/reflection.h"
#include "entity/component/editor_tags.h"
#include "imgui.h"
#include "imgui/font_awesome.h"
#include "widget/dialog_open.h"
#include "level/scene_manager.h"
#include "render/renderer_3d.h"
#include "render/renderer_pp.h"
#include "project/project.h"

using namespace erwin;

namespace editor
{

InspectorWidget::InspectorWidget() : Widget("Inspector", true) {}

void InspectorWidget::entity_tab()
{
    auto& scene = scn::current();
    if(!scene.is_loaded())
        return;

    scene.view<SelectedTag, ComponentDescription>(entt::exclude<NonEditableTag>)
        .each([&scene](auto e, auto& desc) {
            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
            if(ImGui::TreeNode("Properties"))
            {
                ImGui::Text("EntityID: %u", e);

                // TODO: Use a resize callback to wire the InputText to the string directly
                static char name_buf[128] = "";
                if(ImGui::InputTextWithHint("Name", "rename entity", name_buf, IM_ARRAYSIZE(name_buf)))
                    desc.name = name_buf;

                ImGui::TreePop();
                ImGui::Separator();
            }

            ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
            if(ImGui::TreeNode("Components"))
            {
                scene.visit_entity(e, [&scene, e](uint32_t reflected_type, void* data) {
                    // Don't let special editor components be editable this way
                    if(is_hidden_from_inspector(reflected_type))
                        return;

                    const char* component_name =
                        entt::resolve_id(reflected_type).prop("name"_hs).value().cast<const char*>();
                    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
                    if(ImGui::TreeNode(component_name))
                    {
                        // Basic controls over this component
                        if(!scene.is_runtime())
                        {
                            ImGui::SameLine(ImGui::GetWindowWidth() - 50);
                            if(ImGui::Button(W_ICON(WINDOW_CLOSE)))
                            {
                                scene.mark_for_removal(e, reflected_type);
                                KLOG("editor", 1) << "Removed component " << component_name << " from entity "
                                                  << static_cast<unsigned long>(e) << std::endl;
                                ImGui::TreePop();
                                return;
                            }
                        }

                        // Invoke GUI for this component
                        invoke(W_METAFUNC_INSPECTOR_GUI, reflected_type, data, e, static_cast<void*>(&scene));
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
                    for(auto&& [reflected, name] : component_names)
                    {
                        // If entity already has component, do not show in combo
                        if(invoke(W_METAFUNC_HAS_COMPONENT, reflected, scene.get_registry(), e).template cast<bool>())
                            continue;

                        if(ImGui::Selectable(name.c_str()))
                        {
                            invoke(W_METAFUNC_CREATE_COMPONENT, reflected, scene.get_registry(), e);
                            KLOG("editor", 1) << "Added " << component_names.at(reflected) << " to entity "
                                              << static_cast<unsigned long>(e) << std::endl;
                            break;
                        }
                    }
                    ImGui::EndPopup();
                }

                ImGui::TreePop();
            }
        });
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

        auto& scene = scn::current();
        // BUGFIX: During runtime: Unload -> Load another -> Reload first one -> crash
        // Crash happens because of an out of range exception in environment resource cache
        // For now I just disable environment loading during runtime
        if(!scene.is_runtime())
        {
            // Load environment from HDR equirectangular texture file
            if(ImGui::Button("Load"))
                editor::dialog::show_open("ChooseHDRDlgKey", "Choose HDR file", ".hdr", editor::project::asset_dir(editor::DK::HDR).absolute());

            editor::dialog::on_open("ChooseHDRDlgKey", [&scene](const fs::path& filepath)
            {
                scene.load_hdr_environment(WPath("res", filepath));
            });
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
            PostProcessingRenderer::on_imgui_render(); // TODO: decouple
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

} // namespace editor