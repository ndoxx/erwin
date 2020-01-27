#include "editor/widget_inspector.h"
#include "editor/scene.h"
#include "entity/entity_manager.h"
#include "core/application.h"
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
    auto& scene = Application::SCENE();

    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("Properties"))
    {
        auto& desc = scene.entities[scene.selected_entity_idx];

        ImGui::Text("EntityID: %lu", desc.id);

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
        Application::ECS().inspector_GUI(scene.entities[scene.selected_entity_idx].id);
        ImGui::TreePop();
    }
}

void InspectorWidget::postproc_tab()
{
    auto& scene = Application::SCENE();

	static bool enable_chromatic_aberration  = true;
	static bool enable_exposure_tone_mapping = true;
	static bool enable_saturation            = true;
	static bool enable_contrast              = true;
	static bool enable_gamma                 = true;
	static bool enable_vibrance              = true;

    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("Chromatic aberration"))
    {
		if(ImGui::Checkbox("Enable##en_ca", &enable_chromatic_aberration))
			scene.post_processing.set_flag_enabled(PP_EN_CHROMATIC_ABERRATION, enable_chromatic_aberration);

        ImGui::SliderFloat("Shift",     &scene.post_processing.ca_shift, 0.0f, 10.0f);
        ImGui::SliderFloat("Magnitude", &scene.post_processing.ca_strength, 0.0f, 1.0f);
        ImGui::TreePop();
        ImGui::Separator();
    }
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("Tone mapping"))
    {
		if(ImGui::Checkbox("Enable##en_tm", &enable_exposure_tone_mapping))
			scene.post_processing.set_flag_enabled(PP_EN_EXPOSURE_TONE_MAPPING, enable_exposure_tone_mapping);

        ImGui::SliderFloat("Exposure", &scene.post_processing.tm_exposure, 0.1f, 5.0f);
        ImGui::TreePop();
        ImGui::Separator();
    }
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("Correction"))
    {
		if(ImGui::Checkbox("Enable##en_sat", &enable_saturation))
			scene.post_processing.set_flag_enabled(PP_EN_SATURATION, enable_saturation);

        ImGui::SliderFloat("Saturation", &scene.post_processing.cor_saturation, 0.0f, 2.0f);
		if(ImGui::Checkbox("Enable##en_cnt", &enable_contrast))
			scene.post_processing.set_flag_enabled(PP_EN_CONTRAST, enable_contrast);

        ImGui::SliderFloat("Contrast", &scene.post_processing.cor_contrast, 0.0f, 2.0f);
		if(ImGui::Checkbox("Enable##en_gam", &enable_gamma))
			scene.post_processing.set_flag_enabled(PP_EN_GAMMA, enable_gamma);

        ImGui::SliderFloat3("Gamma", (float*)&scene.post_processing.cor_gamma, 1.0f, 2.0f);
        ImGui::TreePop();
        ImGui::Separator();
    }
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("Vibrance"))
    {
		if(ImGui::Checkbox("Enable##en_vib", &enable_vibrance))
			scene.post_processing.set_flag_enabled(PP_EN_VIBRANCE, enable_vibrance);

        ImGui::SliderFloat("Strength", &scene.post_processing.vib_strength, -1.0f, 2.0f);
        ImGui::SliderFloat3("Balance", (float*)&scene.post_processing.vib_balance, 0.0f, 1.0f);
        ImGui::TreePop();
        ImGui::Separator();
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
			postproc_tab();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}


} // namespace editor