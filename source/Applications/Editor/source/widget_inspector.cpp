#include "widget_inspector.h"
#include "game/scene.h"
#include "game/game_components.h"
#include "erwin.h"
#include "imgui.h"

using namespace erwin;

namespace editor
{

InspectorWidget::InspectorWidget(game::Scene& scene, erwin::EntityManager& emgr):
Widget("Inspector", true),
scene_(scene),
entity_manager_(emgr)
{

}

InspectorWidget::~InspectorWidget()
{

}

void InspectorWidget::environment_tab()
{
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("Directional light"))
    {
        entity_manager_.inspector_GUI(scene_.directional_light);
        ImGui::TreePop();
    }
}

void InspectorWidget::postproc_tab()
{
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
			scene_.post_processing.set_flag_enabled(PP_EN_CHROMATIC_ABERRATION, enable_chromatic_aberration);

        ImGui::SliderFloat("Shift",     &scene_.post_processing.ca_shift, 0.0f, 10.0f);
        ImGui::SliderFloat("Magnitude", &scene_.post_processing.ca_strength, 0.0f, 1.0f);
        ImGui::TreePop();
        ImGui::Separator();
    }
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("Tone mapping"))
    {
		if(ImGui::Checkbox("Enable##en_tm", &enable_exposure_tone_mapping))
			scene_.post_processing.set_flag_enabled(PP_EN_EXPOSURE_TONE_MAPPING, enable_exposure_tone_mapping);

        ImGui::SliderFloat("Exposure", &scene_.post_processing.tm_exposure, 0.1f, 5.0f);
        ImGui::TreePop();
        ImGui::Separator();
    }
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("Correction"))
    {
		if(ImGui::Checkbox("Enable##en_sat", &enable_saturation))
			scene_.post_processing.set_flag_enabled(PP_EN_SATURATION, enable_saturation);

        ImGui::SliderFloat("Saturation", &scene_.post_processing.cor_saturation, 0.0f, 2.0f);
		if(ImGui::Checkbox("Enable##en_cnt", &enable_contrast))
			scene_.post_processing.set_flag_enabled(PP_EN_CONTRAST, enable_contrast);

        ImGui::SliderFloat("Contrast", &scene_.post_processing.cor_contrast, 0.0f, 2.0f);
		if(ImGui::Checkbox("Enable##en_gam", &enable_gamma))
			scene_.post_processing.set_flag_enabled(PP_EN_GAMMA, enable_gamma);

        ImGui::SliderFloat3("Gamma", (float*)&scene_.post_processing.cor_gamma, 1.0f, 2.0f);
        ImGui::TreePop();
        ImGui::Separator();
    }
    ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
    if(ImGui::TreeNode("Vibrance"))
    {
		if(ImGui::Checkbox("Enable##en_vib", &enable_vibrance))
			scene_.post_processing.set_flag_enabled(PP_EN_VIBRANCE, enable_vibrance);

        ImGui::SliderFloat("Strength", &scene_.post_processing.vib_strength, -1.0f, 2.0f);
        ImGui::SliderFloat3("Balance", (float*)&scene_.post_processing.vib_balance, 0.0f, 1.0f);
        ImGui::TreePop();
        ImGui::Separator();
    }
}

void InspectorWidget::on_imgui_render()
{
	static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_Reorderable;

	if(ImGui::BeginTabBar("InspectorTabs", tab_bar_flags))
	{
		if(ImGui::BeginTabItem("Environment"))
		{
			environment_tab();
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