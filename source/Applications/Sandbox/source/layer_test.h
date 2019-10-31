#pragma once

#include <iostream>
#include <iomanip>
#include <bitset>

#include "erwin.h"

// #include "render/buffer.h"
// #include "render/shader.h"
// #include "platform/ogl_shader.h"
// #include "platform/ogl_texture.h"
#include "render/WIP/renderer_2d.h"
#include "memory/memory_utils.h"

using namespace erwin;
using namespace erwin::WIP;

class LayerTest: public Layer
{
public:
	LayerTest(): Layer("TestLayer"), camera_ctl_(1280.f/1024.f, 1.f)
	{
	    memory::hex_dump_highlight(0xf0f0f0f0, WCB(100,0,0));
	    memory::hex_dump_highlight(0x0f0f0f0f, WCB(0,0,100));
	    memory::hex_dump_highlight(0xd0d0d0d0, WCB(200,100,0));
	}

	~LayerTest() = default;

	virtual void on_imgui_render() override
	{

	}

	virtual void on_attach() override
	{
		WIP::Renderer2D::init();
		atlas_.load("textures/atlas/set1.cat");
		WIP::Renderer2D::register_atlas("set1"_h, atlas_);

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

	virtual void on_detach() override
	{
		WIP::Renderer2D::shutdown();
	}


protected:
	virtual void on_update(GameClock& clock) override
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

		static int n_quads = 50;
		float scale = 1.f/(n_quads-1);
		WIP::Renderer2D::begin_pass(pass_state, camera_ctl_.get_camera());
		for(int xx=0; xx<n_quads; ++xx)
		{
			float x_pos = -1.f + 2.f*xx/(n_quads-1);
			for(int yy=0; yy<n_quads; ++yy)
			{
				float y_pos = -1.f + 2.f*yy/(n_quads-1);
				hash_t tile = tiles_.at((yy/3 + xx/5)%(tiles_.size()-1));
				WIP::Renderer2D::draw_quad({x_pos,y_pos},{scale,scale},atlas_.get_uv(tile),"set1"_h);
			}
		}
		WIP::Renderer2D::end_pass();
		WIP::Renderer2D::flush();
	}

	virtual bool on_event(const MouseButtonEvent& event) override
	{
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
	OrthographicCamera2DController camera_ctl_;
	WIP::TextureAtlas atlas_;
	std::vector<hash_t> tiles_;
	float tt_ = 0.f;

	FramebufferHandle fb_handle_;
};