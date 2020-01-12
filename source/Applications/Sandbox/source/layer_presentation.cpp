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
    pp_data_.set_flag_enabled(PP_EN_FXAA, enable_fxaa_);
}

void PresentationLayer::on_render()
{
	PostProcessingRenderer::reset_sequence();

	if(enable_bloom_)
		if(bloom_alt_)
			PostProcessingRenderer::bloom_pass_alt("fb_forward"_h, 1, get_layer_id());
		else
			PostProcessingRenderer::bloom_pass("fb_forward"_h, 1, get_layer_id());


	if(enable_3d_forward_)
	{
    	pp_data_.set_flag_enabled(PP_EN_BLOOM, enable_bloom_);
		PostProcessingRenderer::combine("fb_forward"_h, 0, pp_data_, get_layer_id());
	}
    if(enable_2d_batched_)
    {
    	pp_data_.clear_flag(PP_EN_BLOOM);
		PostProcessingRenderer::combine("fb_2d_raw"_h, 0, pp_data_, get_layer_id());
    }
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
