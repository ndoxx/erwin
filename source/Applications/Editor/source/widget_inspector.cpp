#include "widget_inspector.h"
#include "game/scene.h"
#include "erwin.h"
#include "imgui.h"

using namespace erwin;

namespace editor
{

InspectorWidget::InspectorWidget(game::Scene& scene):
Widget("Inspector", true),
scene_(scene)
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
    	static float inclination_deg   = 90.0f;
    	static float arg_periapsis_deg = 160.0f;
        if(ImGui::SliderFloat("Inclination", &inclination_deg, 0.0f, 180.0f))
        	scene_.directional_light.set_position(inclination_deg, arg_periapsis_deg);
        if(ImGui::SliderFloat("Arg. periapsis", &arg_periapsis_deg, 0.0f, 360.0f))
        	scene_.directional_light.set_position(inclination_deg, arg_periapsis_deg);

        ImGui::SliderFloat("Brightness", &scene_.directional_light.brightness, 0.0f, 30.0f);
        ImGui::SliderFloat("Ambient str.", &scene_.directional_light.ambient_strength, 0.0f, 0.5f);
        ImGui::ColorEdit3("Color", (float*)&scene_.directional_light.color);
        ImGui::ColorEdit3("Amb. color", (float*)&scene_.directional_light.ambient_color);
        ImGui::SliderFloat("App. diameter", &scene_.sun_material_data_.scale, 0.1f, 0.4f);

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