// #include "glad/glad.h"
#include <memory>
#include <iostream>
#include <fstream>
#include "erwin.h"

// #include "render/buffer.h"
// #include "render/shader.h"
// #include "platform/ogl_shader.h"
// #include "platform/ogl_texture.h"

#include "render/texture_atlas.h"

using namespace erwin;

class LayerBatch2D: public Layer
{
public:
	LayerBatch2D():
	Layer("LayerBatch2D"),
	camera_ctl_(1280.f/1024.f, 1.f)
	{
		EVENTBUS.subscribe(this, &LayerBatch2D::on_window_resize_event);
		EVENTBUS.subscribe(this, &LayerBatch2D::on_mouse_scroll_event);
	}

	~LayerBatch2D() = default;

	virtual bool on_event(const MouseButtonEvent& event) override
	{
		DLOGN("event") << get_name() << " -> Handled event: " << event << std::endl;
		return true;
	}

	virtual void on_imgui_render() override
	{
		static const char* rd_items[] = {"Batch renderer", "Instanced renderer"};
	    
	    ImGui::Begin("BatchRenderer2D");

        	if(ImGui::Checkbox("Profile", &enable_profiling_))
        		renderer_2D_->set_profiling_enabled(enable_profiling_);

            ImGui::Separator();

            static int s_renderer_impl = 0;
            ImGui::WCombo("##renderersel", "Renderer", s_renderer_impl, 2, rd_items);
            if(ImGui::Button("Swap implementation"))
            {
            	switch(s_renderer_impl)
            	{
            		case 0: renderer_2D_ = std::make_unique<BatchRenderer2D>(batch_size_); break;
            		case 1: renderer_2D_ = std::make_unique<InstanceRenderer2D>(batch_size_); break;
            		default: break;
            	}
            	renderer_2D_->set_profiling_enabled(enable_profiling_);
            }
            ImGui::Separator();

            ImGui::SliderInt("Grid size", &len_grid_, 10, 500);
            if(ImGui::SliderInt("Batch size", &batch_size_, 200, 10000))
            {
				renderer_2D_->set_batch_size(batch_size_);
            }
        	ImGui::Text("Drawing %d squares.", len_grid_*len_grid_);
        	ImGui::Checkbox("Trippy mode", &trippy_mode_);

	    ImGui::End();

	    if(enable_profiling_)
	    {
	    	ImGui::Begin("Stats");
            	ImGui::Text("#Batches: %d", render_stats_.batches);
            	ImGui::Separator();
            	ImGui::PlotVar("Draw time (µs)", render_stats_.render_time, 0.0f, 7000.f);
            	ImGui::PlotVar("FPS (Hz)", fps_, 0.0f, 100.f);
	    	
	            if(++frame_cnt_>200)
	            {
	                frame_cnt_ = 0;
	                ImGui::PlotVarFlushOldEntries();
	            }
	    	ImGui::End();
	    }
	}

	virtual void on_attach() override
	{
		renderer_2D_ = std::make_unique<BatchRenderer2D>(batch_size_);
		// atlas_.load("textures/atlas/set2.png");
		atlas_.load("textures/atlas/set2.cat");

		// List of random sub-textures to use
		tiles_ =
		{
			"stonea32"_h,
			"pavingd64"_h,
			"paneling64"_h,
			"dirt32"_h,
			"pavingd32"_h,
			"rockb32"_h,
			"rockc64"_h,
			"rocka64"_h,
			"thatchb64"_h,
			"rockd32"_h,
			"sand64"_h,
			"planks64"_h,
			"rocke64"_h,
			"planks32"_h,
			"clover32"_h,
			"grass32"_h,
			"stonewalld64"_h,
			"pavinge64"_h,
			"stonec64"_h,
			"snow64"_h,
			"rockb64"_h
		};

		//dirt_tex_ = Texture2D::create("textures/dirt01_albedo.png");
		//Renderer2D::shader_bank.load("shaders/tex_shader.glsl");
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

		renderer_2D_->begin_scene(camera_ctl_.get_camera(), atlas_.get_texture());
		{
			RenderState render_state;
			render_state.render_target = RenderTarget::Default;
			render_state.rasterizer_state = CullMode::None;
			render_state.blend_state = BlendState::Opaque;
			renderer_2D_->submit(render_state);
/*
			ShaderParameters sq_params;
			sq_params.set_texture_slot("us_albedo"_h, dirt_tex_);
			renderer_2D_->submit(sq_va_, "tex_shader"_h, sq_params);
*/
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

					hash_t tile = tiles_.at((xx+yy)%(tiles_.size()-1));
					renderer_2D_->draw_quad({pos_x,pos_y}, {xx_scale,yy_scale}, atlas_.get_uv(tile));
				}
			}
		}
		renderer_2D_->end_scene();

		render_stats_ = renderer_2D_->get_stats();
	}

bool on_window_resize_event(const WindowResizeEvent& event)
{
	camera_ctl_.on_window_resize_event(event);
	return false;
}

bool on_mouse_scroll_event(const MouseScrollEvent& event)
{
	camera_ctl_.on_mouse_scroll_event(event);
	return false;
}

private:
	std::unique_ptr<Renderer2D> renderer_2D_;
	TextureAtlas atlas_;
	OrthographicCamera2DController camera_ctl_;

	std::vector<hash_t> tiles_;

	RenderStats render_stats_;
	bool enable_profiling_ = false;
	uint32_t frame_cnt_ = 0;
	float fps_ = 60.f;
	float tt_ = 0.f;

	int len_grid_ = 100;
	int batch_size_ = 8192;
	bool trippy_mode_ = false;
};

class Sandbox: public Application
{
public:
	Sandbox()
	{
		EVENTBUS.subscribe(this, &Sandbox::on_keyboard_event);

		filesystem::set_asset_dir("source/Applications/Sandbox/assets");
		push_layer(new LayerBatch2D());
	}

	~Sandbox()
	{

	}

	bool on_keyboard_event(const KeyboardEvent& e)
	{
		// Terminate on ESCAPE
		if(e.pressed && e.key == keymap::WKEY::ESCAPE)
			EVENTBUS.publish(WindowCloseEvent());

		return false;
	}
};

Application* erwin::create_application()
{
	return new Sandbox();
}
