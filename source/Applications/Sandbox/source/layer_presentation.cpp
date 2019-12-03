#include "layer_presentation.h"

#include <iostream>
#include <iomanip>
#include <bitset>

using namespace erwin;

PresentationLayer::PresentationLayer(): Layer("PresentationLayer")
{

}

void PresentationLayer::on_imgui_render()
{
    ImGui::Begin("PostProcessing");
        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Chromatic aberration"))
        {
    		ImGui::Checkbox("##en_ca", &enable_chromatic_aberration_);

            ImGui::SliderFloat("Shift",     &pp_data_.ca_shift, 0.0f, 10.0f);
            ImGui::SliderFloat("Magnitude", &pp_data_.ca_strength, 0.0f, 1.0f);
            ImGui::TreePop();
            ImGui::Separator();
        }
        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Tone mapping"))
        {
    		ImGui::Checkbox("##en_tm", &enable_exposure_tone_mapping_); ImGui::SameLine();
            ImGui::SliderFloat("Exposure", &pp_data_.tm_exposure, 0.1f, 5.0f);
            ImGui::TreePop();
            ImGui::Separator();
        }
        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Correction"))
        {
    		ImGui::Checkbox("##en_sat", &enable_saturation_); ImGui::SameLine();
            ImGui::SliderFloat("Saturation",     &pp_data_.cor_saturation, 0.0f, 2.0f);
    		ImGui::Checkbox("##en_cnt", &enable_contrast_); ImGui::SameLine();
            ImGui::SliderFloat("Contrast",       &pp_data_.cor_contrast, 0.0f, 2.0f);
    		ImGui::Checkbox("##en_gam", &enable_gamma_); ImGui::SameLine();
            ImGui::SliderFloat3("Gamma", (float*)&pp_data_.cor_gamma, 1.0f, 2.0f);
            ImGui::TreePop();
            ImGui::Separator();
        }
        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Vibrance"))
        {
    		ImGui::Checkbox("##en_vib", &enable_vibrance_);

            ImGui::SliderFloat("Strength",         &pp_data_.vib_strength, -1.0f, 2.0f);
            ImGui::SliderFloat3("Balance", (float*)&pp_data_.vib_balance, 0.0f, 1.0f);
            ImGui::TreePop();
            ImGui::Separator();
        }
    ImGui::End();
}

void PresentationLayer::on_attach()
{

}

void PresentationLayer::on_detach()
{

}

void PresentationLayer::on_update(GameClock& clock)
{
	float dt = clock.get_frame_duration();
	tt_ += dt;
	if(tt_>=5.f)
		tt_ = 0.f;

    pp_data_.set_flag_enabled(PP_EN_CHROMATIC_ABERRATION, enable_chromatic_aberration_);
    pp_data_.set_flag_enabled(PP_EN_EXPOSURE_TONE_MAPPING, enable_exposure_tone_mapping_);
    pp_data_.set_flag_enabled(PP_EN_VIBRANCE, enable_vibrance_);
    pp_data_.set_flag_enabled(PP_EN_SATURATION, enable_saturation_);
    pp_data_.set_flag_enabled(PP_EN_CONTRAST, enable_contrast_);
    pp_data_.set_flag_enabled(PP_EN_GAMMA, enable_gamma_);
    
	PassState pp_pass_state;
	pp_pass_state.rasterizer_state.cull_mode = CullMode::Back;
	pp_pass_state.blend_state = BlendState::Alpha;
	pp_pass_state.depth_stencil_state.depth_test_enabled = false;
	pp_pass_state.rasterizer_state.clear_color = glm::vec4(0.2f,0.2f,0.2f,1.f);
	PostProcessingRenderer::begin_pass(pp_pass_state, pp_data_);
	if(enable_2d_batched_)
		PostProcessingRenderer::blit("fb_2d_raw"_h);
	if(enable_3d_forward_)
		PostProcessingRenderer::blit("fb_forward"_h);
	PostProcessingRenderer::end_pass();
}

bool PresentationLayer::on_event(const MouseButtonEvent& event)
{
	return false;
}

bool PresentationLayer::on_event(const WindowResizeEvent& event)
{
	return false;
}

bool PresentationLayer::on_event(const MouseScrollEvent& event)
{
	return false;
}
