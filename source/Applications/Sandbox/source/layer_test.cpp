#include "layer_test.h"

#include <iostream>
#include <iomanip>
#include <bitset>

#include "erwin.h"

// #include "render/buffer.h"
// #include "render/shader.h"
// #include "platform/ogl_shader.h"
// #include "platform/ogl_texture.h"
#include "memory/memory_utils.h"

using namespace erwin;

LayerTest::LayerTest(): Layer("TestLayer"), camera_ctl_(1280.f/1024.f, 1.f)
{
    memory::hex_dump_highlight(0xf0f0f0f0, WCB(100,0,0));
    memory::hex_dump_highlight(0x0f0f0f0f, WCB(0,0,100));
    memory::hex_dump_highlight(0xd0d0d0d0, WCB(200,100,0));
}

void LayerTest::on_imgui_render()
{
    ImGui::Begin("BatchRenderer2D");
    	if(ImGui::Checkbox("Profile", &enable_profiling_))
    		MainRenderer::set_profiling_enabled(enable_profiling_);

    	ImGui::Separator();

        ImGui::SliderInt("Grid size", &len_grid_, 10, 500);
    	ImGui::Text("Drawing %d squares.", len_grid_*len_grid_);
    	ImGui::Checkbox("Trippy mode", &trippy_mode_);
    ImGui::End();

    if(enable_profiling_)
    {
    	static uint32_t s_frame_cnt = 0;
    	const auto& mr_stats = MainRenderer::get_stats();
    	uint32_t rd2d_draw_calls = Renderer2D::get_draw_call_count();
    	ImGui::Begin("Stats");
        	ImGui::Text("#Draw calls: %d", rd2d_draw_calls);
        	ImGui::Separator();
        	ImGui::PlotVar("Draw time (µs)", mr_stats.render_time, 0.0f, 7000.f);
    	
            if(++s_frame_cnt>200)
            {
                s_frame_cnt = 0;
                ImGui::PlotVarFlushOldEntries();
            }
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

void LayerTest::on_attach()
{
	atlas_.load("textures/atlas/set1.cat");
	Renderer2D::register_atlas("set1"_h, atlas_);

	FramebufferLayout layout =
	{
		{"albedo"_h, ImageFormat::RGBA8, MIN_LINEAR | MAG_NEAREST, TextureWrap::CLAMP_TO_EDGE}
	};
	fb_handle_ = FramebufferPool::create_framebuffer("fb_2d_raw"_h, make_scope<FbRatioConstraint>(), layout, false);

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
}

void LayerTest::on_detach()
{

}

void LayerTest::on_update(GameClock& clock)
{
	float dt = clock.get_frame_duration();
	tt_ += dt;
	if(tt_>=5.f)
		tt_ = 0.f;

	camera_ctl_.update(clock);

	PassState pass_state;
	pass_state.rasterizer_state.cull_mode = CullMode::Back;
	pass_state.rasterizer_state.clear_color = glm::vec4(0.2f,0.2f,0.2f,1.f);
	pass_state.blend_state = BlendState::Opaque;

	// Draw a grid of quads
	Renderer2D::begin_pass(MainRenderer::default_render_target(), pass_state, camera_ctl_.get_camera());
	for(int xx=0; xx<len_grid_; ++xx)
	{
		float xx_offset = trippy_mode_ ? 3.0f/len_grid_ * cos(2*2*M_PI*xx/(1.f+len_grid_))*sin(0.2f*2*M_PI*tt_) : 0.f;
		float xx_scale = trippy_mode_ ? 1.0f/len_grid_ * (0.5f+sin(0.2f*2*M_PI*tt_)*sin(0.2f*2*M_PI*tt_)) : 1.f/float(len_grid_-1);
		float pos_x = -1.f + 2.f*xx/float(len_grid_-1) + xx_offset;

		for(int yy=0; yy<len_grid_; ++yy)
		{
			float yy_offset = trippy_mode_ ? 3.0f/len_grid_ * sin(2*2*M_PI*yy/(1.f+len_grid_))*cos(0.2f*2*M_PI*tt_) : 0.f;
			float yy_scale = trippy_mode_ ? 1.0f/len_grid_ * (0.5f+sin(0.2f*2*M_PI*tt_)*sin(0.2f*2*M_PI*tt_)) : 1.f/float(len_grid_-1);
			float pos_y = -1.f + 2.f*yy/float(len_grid_-1) + yy_offset;

			// hash_t tile = tiles_.at((xx+yy)%(tiles_.size()-1));
			hash_t tile = tiles_.at((yy/3 + xx/5)%(tiles_.size()-1));
			Renderer2D::draw_quad({pos_x,pos_y}, {xx_scale,yy_scale}, atlas_.get_uv(tile), "set1"_h);
		}
	}
	Renderer2D::end_pass();
}

bool LayerTest::on_event(const MouseButtonEvent& event)
{
	return false;
}

bool LayerTest::on_event(const WindowResizeEvent& event)
{
	camera_ctl_.on_window_resize_event(event);
	return false;
}

bool LayerTest::on_event(const MouseScrollEvent& event)
{
	camera_ctl_.on_mouse_scroll_event(event);
	return false;
}
