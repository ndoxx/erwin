#include "sandbox_app.h"
#include "debug/texture_peek.h"

void Sandbox::on_client_init()
{
	filesystem::set_asset_dir("source/Applications/Sandbox/assets");
	filesystem::set_client_config_dir("source/Applications/Sandbox/config");
	this->add_configuration("client.xml");
}

void Sandbox::on_load()
{
	EVENTBUS.subscribe(this, &Sandbox::on_keyboard_event);

	push_layer(layer_3d_ = new Layer3D());
	push_layer(layer_2d_ = new Layer2D());
	push_overlay(presentation_layer_ = new PresentationLayer());
	push_overlay(debug_layer_        = new DebugLayer());

    enable_runtime_profiling_ = cfg::get<bool>("erwin.profiling.runtime_session_enabled"_h, false);
#ifdef W_DEBUG
    TexturePeek::register_framebuffer("LBuffer");
    TexturePeek::register_framebuffer("SpriteBuffer");
#endif
    
    set_layer_enabled(0, layer3d_enabled_);
    set_layer_enabled(1, layer2d_enabled_);
}

bool Sandbox::on_keyboard_event(const KeyboardEvent& e)
{
	// Terminate on ESCAPE
	if(e.pressed && e.key == keymap::WKEY::ESCAPE)
		EVENTBUS.publish(WindowCloseEvent());

	// Toggle ImGui layer on TAB
	if(e.pressed && e.key == keymap::WKEY::TAB)
		toggle_imgui_layer();

	return false;
}

void Sandbox::on_imgui_render()
{
	static bool show_app_texture_peek_window = false;
	static bool show_app_profiling_window = false;
	static bool show_app_layer_config_window = false;
	static bool show_app_post_processing_window = false;
	static bool show_app_render_stats_window = false;
	static bool show_app_lighting_window = false;

	if(ImGui::BeginMainMenuBar())
	{
    	if(ImGui::BeginMenu("Debug"))
    	{
#ifdef W_PROFILE
        	ImGui::MenuItem("Render stats", NULL, &show_app_render_stats_window);
        	ImGui::MenuItem("Profiling", NULL, &show_app_profiling_window);
#endif
        	ImGui::MenuItem("Texture peek", NULL, &show_app_texture_peek_window);
        	ImGui::EndMenu();
        }
    	if(ImGui::BeginMenu("Layers"))
    	{
            if(ImGui::Checkbox("3D Layer", &layer3d_enabled_)) toggle_layer_3d();
            if(ImGui::Checkbox("2D Layer", &layer2d_enabled_)) toggle_layer_2d();
        	ImGui::MenuItem("Configuration", NULL, &show_app_layer_config_window);
        	ImGui::EndMenu();
        }
    	if(ImGui::BeginMenu("Post processing"))
        {
            ImGui::Checkbox("Bloom", &presentation_layer_->enable_bloom_);
            ImGui::Checkbox("Bloom Alt.", &presentation_layer_->bloom_alt_);
            ImGui::Checkbox("FXAA", &presentation_layer_->enable_fxaa_);
        	ImGui::MenuItem("Tweaks", NULL, &show_app_post_processing_window);
        	ImGui::EndMenu();
        }
    	if(ImGui::BeginMenu("Scene"))
        {
        	ImGui::MenuItem("Lighting", NULL, &show_app_lighting_window);
        	ImGui::EndMenu();
        }
    	ImGui::EndMainMenuBar();
	}

	TexturePeek::set_enabled(show_app_texture_peek_window);

	if(show_app_layer_config_window)    window_layer_config(&show_app_layer_config_window);
	if(show_app_post_processing_window) window_post_processing(&show_app_post_processing_window);
	if(show_app_lighting_window)        window_lighting(&show_app_lighting_window);
	if(show_app_texture_peek_window)    TexturePeek::on_imgui_render(&show_app_texture_peek_window);
#ifdef W_PROFILE
    Renderer::set_profiling_enabled(show_app_render_stats_window);
	if(show_app_profiling_window)       window_profiling(&show_app_profiling_window);
	if(show_app_render_stats_window)    window_render_stats(&show_app_render_stats_window);
#endif

/*
    static float sigma = 1.f;
    static int size = 1;
    if(ImGui::Begin("DBG",nullptr))
    {
        if(ImGui::SliderFloat("Sigma", &sigma, 0.01f, 2.0f))
            ForwardRenderer::set_gaussian_kernel(2*size+1,sigma);
        if(ImGui::SliderInt("KSize", &size, 1, 7))
            ForwardRenderer::set_gaussian_kernel(2*size+1,sigma);


        ImGui::End();
    }*/
}

void Sandbox::toggle_layer_3d()
{
	layer_3d_->set_enabled(layer3d_enabled_);
	presentation_layer_->enable_forward_rendering(layer3d_enabled_);
}

void Sandbox::toggle_layer_2d()
{
	layer_2d_->set_enabled(layer2d_enabled_);
	presentation_layer_->enable_2d_rendering(layer2d_enabled_);
}

void Sandbox::window_layer_config(bool* p_open)
{
    if(ImGui::Begin("Layer configuration", p_open))
    {
    	if(layer_2d_->is_enabled())
    	{
        	ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
	        if(ImGui::TreeNode("2D Layer"))
	        {
		        ImGui::SliderInt("Grid size", &layer_2d_->len_grid_, 10, 500);
		    	ImGui::Text("Drawing %d squares.", layer_2d_->len_grid_*layer_2d_->len_grid_);
		    	ImGui::Checkbox("Trippy mode", &layer_2d_->trippy_mode_);
		    }
		}
    	ImGui::End();
    }
}

void Sandbox::window_post_processing(bool* p_open)
{
    if(ImGui::Begin("Post-processing", p_open))
    {
        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Chromatic aberration"))
        {
    		ImGui::Checkbox("##en_ca", &presentation_layer_->enable_chromatic_aberration_);

            ImGui::SliderFloat("Shift",     &presentation_layer_->pp_data_.ca_shift, 0.0f, 10.0f);
            ImGui::SliderFloat("Magnitude", &presentation_layer_->pp_data_.ca_strength, 0.0f, 1.0f);
            ImGui::TreePop();
            ImGui::Separator();
        }
        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Tone mapping"))
        {
    		ImGui::Checkbox("##en_tm", &presentation_layer_->enable_exposure_tone_mapping_); ImGui::SameLine();
            ImGui::SliderFloat("Exposure", &presentation_layer_->pp_data_.tm_exposure, 0.1f, 5.0f);
            ImGui::TreePop();
            ImGui::Separator();
        }
        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Correction"))
        {
    		ImGui::Checkbox("##en_sat", &presentation_layer_->enable_saturation_); ImGui::SameLine();
            ImGui::SliderFloat("Saturation", &presentation_layer_->pp_data_.cor_saturation, 0.0f, 2.0f);
    		ImGui::Checkbox("##en_cnt", &presentation_layer_->enable_contrast_); ImGui::SameLine();
            ImGui::SliderFloat("Contrast", &presentation_layer_->pp_data_.cor_contrast, 0.0f, 2.0f);
    		ImGui::Checkbox("##en_gam", &presentation_layer_->enable_gamma_); ImGui::SameLine();
            ImGui::SliderFloat3("Gamma", (float*)&presentation_layer_->pp_data_.cor_gamma, 1.0f, 2.0f);
            ImGui::TreePop();
            ImGui::Separator();
        }
        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Vibrance"))
        {
    		ImGui::Checkbox("##en_vib", &presentation_layer_->enable_vibrance_);

            ImGui::SliderFloat("Strength", &presentation_layer_->pp_data_.vib_strength, -1.0f, 2.0f);
            ImGui::SliderFloat3("Balance", (float*)&presentation_layer_->pp_data_.vib_balance, 0.0f, 1.0f);
            ImGui::TreePop();
            ImGui::Separator();
        }
    	ImGui::End();
    }
}

void Sandbox::window_lighting(bool* p_open)
{
    if(ImGui::Begin("Lighting", p_open))
    {
        ImGui::SetNextTreeNodeOpen(true, ImGuiCond_Once);
        if(ImGui::TreeNode("Directional light"))
        {
        	static float inclination_deg   = 90.0f;
        	static float arg_periapsis_deg = 160.0f;
            if(ImGui::SliderFloat("Inclination", &inclination_deg, 0.0f, 180.0f))
            	layer_3d_->dir_light_.set_position(inclination_deg, arg_periapsis_deg);
            if(ImGui::SliderFloat("Arg. periapsis", &arg_periapsis_deg, 0.0f, 360.0f))
            	layer_3d_->dir_light_.set_position(inclination_deg, arg_periapsis_deg);

            ImGui::SliderFloat("Brightness", &layer_3d_->dir_light_.brightness, 0.0f, 30.0f);
            ImGui::SliderFloat("Ambient str.", &layer_3d_->dir_light_.ambient_strength, 0.0f, 0.5f);
            ImGui::ColorEdit3("Color", (float*)&layer_3d_->dir_light_.color);
            ImGui::ColorEdit3("Amb. color", (float*)&layer_3d_->dir_light_.ambient_color);

            ImGui::Separator();
            ImGui::SliderFloat("App. diameter", &layer_3d_->sun_material_data_.scale, 0.1f, 0.4f);


            ImGui::TreePop();
        }
    	ImGui::End();
    }
}

#ifdef W_PROFILE
void Sandbox::window_profiling(bool* p_open)
{
    if(ImGui::Begin("Profiling", p_open))
    {
        if(ImGui::Button("Runtime profiling"))
        {
            enable_runtime_profiling_ = !enable_runtime_profiling_;
            W_PROFILE_ENABLE_SESSION(enable_runtime_profiling_);
        }

        ImGui::SameLine();
        if(enable_runtime_profiling_ || frame_profiling_)
            ImGui::TextColored({0.f,1.f,0.f,1.f}, "[*]");
        else
            ImGui::TextColored({1.f,0.f,0.f,1.f}, "[ ]");

        ImGui::Text("Profile next frames");
        ImGui::SliderInt("Frames", &profile_num_frames_, 1, 120);

        if(ImGui::Button("Start"))
        {
            frame_profiling_ = !frame_profiling_;
            frames_counter_ = 0;
            W_PROFILE_ENABLE_SESSION(true);
        }

        ImGui::Separator();
        if(ImGui::Button("Track draw calls"))
        {
            track_draw_calls("draw_calls.json");
        }

    	ImGui::End();
    }
    if(frame_profiling_)
    {
        if(frames_counter_++ == profile_num_frames_)
        {
            frame_profiling_ = false;
            W_PROFILE_ENABLE_SESSION(false);
        }
    }
}

void Sandbox::window_render_stats(bool* p_open)
{
    if(ImGui::Begin("Stats", p_open))
    {
		const auto& mr_stats = Renderer::get_stats();
		uint32_t rd2d_draw_calls = Renderer2D::get_draw_call_count();

    	ImGui::Text("#Draw calls: %d", rd2d_draw_calls);
    	ImGui::Separator();
    	ImGui::PlotVar("Draw time (µs)", mr_stats.render_time, 0.0f, 7000.f);
        ImGui::PlotVarFlushOldEntries();
		ImGui::End();
	}

/*
	// BUG#2 tracking
	static uint32_t s_frame = 0;
	static int s_displayed = 0;
    if(mr_stats.render_time>1500 && s_displayed<25)
    {
    	DLOGW("render") << "Frame: " << s_frame << std::endl;
    	++s_displayed;
    }
    ++s_frame;
*/
}

#endif