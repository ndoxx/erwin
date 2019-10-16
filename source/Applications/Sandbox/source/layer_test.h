#pragma once

#include "erwin.h"
#include "render/render_device.h"
#include "render/WIP_renderer_2d.h"

using namespace erwin;


class LayerTest: public Layer
{
public:
	LayerTest():
	Layer("LayerTest"),
	camera_ctl_(1280.f/1024.f, 1.f)
	{

	}

	~LayerTest() = default;

	virtual void on_imgui_render() override
	{	    
	    ImGui::Begin("BatchRenderer2D");
	        ImGui::SliderInt("Grid size", &len_grid_, 10, 500);
	        if(ImGui::SliderInt("Batch size", &batch_size_, 200, 10000))
				renderer_2D_->set_batch_size(batch_size_);

	    	ImGui::Text("Drawing %d squares.", len_grid_*len_grid_);
	    	ImGui::Checkbox("Trippy mode", &trippy_mode_);

	    	// Post processing
	        ImGui::Separator();
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

	virtual void on_attach() override
	{
		renderer_2D_ = make_scope<WIP::Renderer2D>(batch_size_);
		// atlas_.load("textures/atlas/set2.png");
		atlas_.load("textures/atlas/set1.cat");

		// List of random sub-textures to use
		tiles_ =
		{
			"thatcha64"_h,
			"stonea64"_h,
			"pavingc64"_h,
			"tileroofa64"_h,
			"pavingd64"_h,
			"paneling64"_h,
			"rockc64"_h,
			"rocka64"_h,
			"thatchb64"_h,
			"sand64"_h,
			"planks64"_h,
			"rocke64"_h,
			"stonewalld64"_h,
			"pavinge64"_h,
			"stonec64"_h,
			"snow64"_h,
			"rockb64"_h,
			"stonewallb64"_h,
			"leaves64"_h,
			"rockd64"_h,
			"woodfloorb64"_h,
			"pavingf64"_h,
			"clover64"_h,
			"tileroofb64"_h,
			"grass64"_h,
			"dirt64"_h,
			"stoneb64"_h,
			"woodfloora64"_h,
			"stonewallc64"_h,
			"pavinga64"_h,
			"rockf64"_h,
			"pavingb64"_h,
			"stonewalla64"_h
		};

		FrameBufferLayout layout =
		{
			{"albedo"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE}
		};
		Gfx::framebuffer_pool->create_framebuffer("fb_2d_raw"_h, make_scope<FbRatioConstraint>(), layout, false);
	}


protected:
	virtual void on_update(GameClock& clock) override
	{
		// Update
		camera_ctl_.update(clock);

		// Render submission
		float dt = clock.get_frame_duration();
		fps_ = 1.f/dt;

		tt_ += dt;
		if(tt_>=5.f)
			tt_ = 0.f;

	    pp_data_.set_flag_enabled(WIP::PP_EN_CHROMATIC_ABERRATION, enable_chromatic_aberration_);
	    pp_data_.set_flag_enabled(WIP::PP_EN_EXPOSURE_TONE_MAPPING, enable_exposure_tone_mapping_);
	    pp_data_.set_flag_enabled(WIP::PP_EN_VIBRANCE, enable_vibrance_);
	    pp_data_.set_flag_enabled(WIP::PP_EN_SATURATION, enable_saturation_);
	    pp_data_.set_flag_enabled(WIP::PP_EN_CONTRAST, enable_contrast_);
	    pp_data_.set_flag_enabled(WIP::PP_EN_GAMMA, enable_gamma_);

		PassState pass_state;
		pass_state.render_target = "fb_2d_raw"_h;
		pass_state.rasterizer_state.cull_mode = CullMode::None;
		pass_state.rasterizer_state.clear_color = glm::vec4(0.2f,0.2f,0.2f,1.f);
		pass_state.blend_state = BlendState::Opaque;

		renderer_2D_->begin_scene(pass_state, camera_ctl_.get_camera(), atlas_.get_texture(), pp_data_);
		{
			// Draw a grid of quads
			for(int xx=0; xx<len_grid_; ++xx)
			{
				float xx_offset = trippy_mode_ ? 3.0f/len_grid_ * cos(2*2*M_PI*xx/(1.f+len_grid_))*sin(0.2f*2*M_PI*tt_) : 0.f;
				float xx_scale = trippy_mode_ ? 1.0f/len_grid_ * (0.5f+sin(0.2f*2*M_PI*tt_)*sin(0.2f*2*M_PI*tt_)) : 2.f/float(len_grid_-1);
				float pos_x = -1.f + 2.f*xx/float(len_grid_-1) + xx_offset;

				for(int yy=0; yy<len_grid_; ++yy)
				{
					float yy_offset = trippy_mode_ ? 3.0f/len_grid_ * sin(2*2*M_PI*yy/(1.f+len_grid_))*cos(0.2f*2*M_PI*tt_) : 0.f;
					float yy_scale = trippy_mode_ ? 1.0f/len_grid_ * (0.5f+sin(0.2f*2*M_PI*tt_)*sin(0.2f*2*M_PI*tt_)) : 2.f/float(len_grid_-1);
					float pos_y = -1.f + 2.f*yy/float(len_grid_-1) + yy_offset;

					// hash_t tile = tiles_.at((xx+yy)%(tiles_.size()-1));
					hash_t tile = tiles_.at((yy/3 + xx/5)%(tiles_.size()-1));
					renderer_2D_->draw_quad({pos_x,pos_y}, {xx_scale,yy_scale}, atlas_.get_uv(tile));
				}
			}
		}
		renderer_2D_->end_scene();

		render_stats_ = renderer_2D_->get_stats();
	}
	virtual bool on_event(const MouseButtonEvent& event) override
	{
		DLOGN("event") << get_name() << " -> Handled event: " << event << std::endl;
		return false;
	}
	virtual bool on_event(const WindowResizeEvent& event) override
	{
		camera_ctl_.on_window_resize_event(event);
		return false;
	}
	virtual bool on_event(const MouseScrollEvent& event) override
	{
		camera_ctl_.on_mouse_scroll_event(event);
		return false;
	}

private:
	WScope<WIP::Renderer2D> renderer_2D_;
	TextureAtlas atlas_;
	OrthographicCamera2DController camera_ctl_;
	std::vector<hash_t> tiles_;

	int len_grid_ = 100;
	int batch_size_ = 8192;
	bool trippy_mode_ = false;

	WIP::PostProcData pp_data_;
	WIP::RenderStats render_stats_;
	bool enable_profiling_ = false;
	uint32_t frame_cnt_ = 0;
	float fps_ = 60.f;
	float tt_ = 0.f;

	bool enable_chromatic_aberration_ = true;
	bool enable_exposure_tone_mapping_ = true;
	bool enable_vibrance_ = true;
	bool enable_saturation_ = true;
	bool enable_contrast_ = true;
	bool enable_gamma_ = true;
};