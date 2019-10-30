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
/*
		auto ubo_handle = rq.create_uniform_buffer("layout_xyz", nullptr, 64, DrawMode::Dynamic);
		auto ssbo_handle = rq.create_shader_storage_buffer("layout_xyz", nullptr, 10*64, DrawMode::Dynamic);
*/
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
		float scale = 1.f/(n_quads);
		WIP::Renderer2D::begin_pass(pass_state, camera_ctl_.get_camera());
		for(int xx=0; xx<n_quads; ++xx)
		{
			float x_pos = -1.f + 2.f*xx/(n_quads-1);
			for(int yy=0; yy<n_quads; ++yy)
			{
				float y_pos = -1.f + 2.f*yy/(n_quads-1);
				WIP::Renderer2D::draw_quad({x_pos,y_pos},{scale,scale},{0.f,0.f,1.f,1.f},0);
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
	float tt_ = 0.f;
};