#include "layer_2d.h"

#include <iostream>
#include <iomanip>
#include <bitset>

#include "erwin.h"

#include "memory/memory_utils.h"
#include "game.h"

using namespace erwin;

Layer2D::Layer2D(): Layer("2DLayer"), camera_ctl_(1280.f/1024.f, 1.f)
{

}

void Layer2D::on_imgui_render()
{

}

void Layer2D::on_attach()
{
	connectivity_atlas_.load("textures/atlas/connectivity.cat");
	Renderer2D::register_atlas("connectivity"_h, connectivity_atlas_);
	dungeon_atlas_.load("textures/atlas/dungeon.cat");
	Renderer2D::register_atlas("dungeon"_h, dungeon_atlas_);

	// List of sub-textures to use
	con_tiles_ =
	{
		"connectivity_00"_h,
		"connectivity_01"_h,
		"connectivity_02"_h,
		"connectivity_03"_h,
		"connectivity_04"_h,
		"connectivity_05"_h,
		"connectivity_06"_h,
		"connectivity_07"_h,
		"connectivity_08"_h,
		"connectivity_09"_h,
		"connectivity_10"_h,
		"connectivity_11"_h,
		"connectivity_12"_h,
		"connectivity_13"_h,
		"connectivity_14"_h,
		"connectivity_15"_h,
	};

	dun_tiles_ =
	{
		"dungeon_c"_h,
		"dungeon_tl"_h,
		"dungeon_t"_h,
		"dungeon_tr"_h,
		"dungeon_r"_h,
		"dungeon_br"_h,
		"dungeon_b"_h,
		"dungeon_bl"_h,
		"dungeon_l"_h,
	};
}

void Layer2D::on_detach()
{

}

static std::vector<glm::vec4> s_tints =
{
	{0.f,0.f,0.f,0.f},
	{0.9275f, 0.1078f, 0.1373f, 1.f},
	{0.2412f, 0.8706f, 0.1961f, 1.f},
	{0.9647f, 0.7529f, 0.1275f, 1.f},
	{0.1255f, 0.2686f, 0.9667f, 1.f},
	{0.3765f, 0.0196f, 0.9333f, 1.f},
};

void Layer2D::on_update(GameClock& clock)
{
	float dt = clock.get_frame_duration();
	tt_ += dt;
	if(tt_>=5.f)
		tt_ = 0.f;

	camera_ctl_.update(clock);

	PassState pass_state;
	pass_state.render_target = FramebufferPool::get_framebuffer("fb_2d_raw"_h);
	pass_state.rasterizer_state.cull_mode = CullMode::Back;
	pass_state.blend_state = BlendState::Alpha;
	pass_state.depth_stencil_state.depth_test_enabled = true;
	pass_state.rasterizer_state.clear_color = glm::vec4(0.2f,0.2f,0.2f,0.f);

	puyo::Dungeon& dungeon = puyo::Game::instance()->get_dungeon();

	float scale_x = 0.1f;
	float scale_y = 0.1f;
	Renderer2D::begin_pass(pass_state, camera_ctl_.get_camera(), get_layer_id());
	for(int yy=dungeon.get_height()-1; yy>=0; --yy)
	{
		float pos_y = -1.f + 2.f*yy*scale_y;
		for(int xx=0; xx<dungeon.get_width(); ++xx)
		{
			float pos_x = -1.f + 2.f*xx*scale_x;
			const puyo::Cell& cell = dungeon.get_cell(xx,yy);
			hash_t tile = con_tiles_.at(cell.connectivity);
			const glm::vec4& tint = s_tints[cell.type];
			Renderer2D::draw_quad({pos_x,pos_y,0.f,1.f}, {scale_x,scale_y}, connectivity_atlas_.get_uv(tile), "connectivity"_h, tint);
			Renderer2D::draw_quad({pos_x,pos_y,-0.1f,1.f}, {scale_x,scale_y}, dungeon_atlas_.get_uv("dungeon_c"_h), "dungeon"_h);
		}
	}
	// Draw dungeon walls
	float pos_xl = -1.f - 2.f*scale_x;
	float pos_xr = -1.f + 2.f*(dungeon.get_width())*scale_x;
	float pos_yb = -1.f - 2.f*scale_y;
	float pos_yt = -1.f + 2.f*(dungeon.get_height())*scale_y;
	for(int yy=0; yy<dungeon.get_height(); ++yy)
	{
		float pos_y = -1.f + 2.f*yy*scale_y;
		Renderer2D::draw_quad({pos_xl,pos_y,-0.1f,1.f}, {scale_x,scale_y}, dungeon_atlas_.get_uv("dungeon_l"_h), "dungeon"_h);
		Renderer2D::draw_quad({pos_xr,pos_y,-0.1f,1.f}, {scale_x,scale_y}, dungeon_atlas_.get_uv("dungeon_r"_h), "dungeon"_h);
	}
	for(int xx=0; xx<dungeon.get_width(); ++xx)
	{
		float pos_x = -1.f + 2.f*xx*scale_y;
		Renderer2D::draw_quad({pos_x,pos_yb,-0.1f,1.f}, {scale_x,scale_y}, dungeon_atlas_.get_uv("dungeon_b"_h), "dungeon"_h);
		Renderer2D::draw_quad({pos_x,pos_yt,-0.1f,1.f}, {scale_x,scale_y}, dungeon_atlas_.get_uv("dungeon_t"_h), "dungeon"_h);
	}
	Renderer2D::draw_quad({pos_xl,pos_yt,-0.1f,1.f}, {scale_x,scale_y}, dungeon_atlas_.get_uv("dungeon_tl"_h), "dungeon"_h);
	Renderer2D::draw_quad({pos_xr,pos_yt,-0.1f,1.f}, {scale_x,scale_y}, dungeon_atlas_.get_uv("dungeon_tr"_h), "dungeon"_h);
	Renderer2D::draw_quad({pos_xl,pos_yb,-0.1f,1.f}, {scale_x,scale_y}, dungeon_atlas_.get_uv("dungeon_bl"_h), "dungeon"_h);
	Renderer2D::draw_quad({pos_xr,pos_yb,-0.1f,1.f}, {scale_x,scale_y}, dungeon_atlas_.get_uv("dungeon_br"_h), "dungeon"_h);

	Renderer2D::end_pass();
}

bool Layer2D::on_event(const MouseButtonEvent& event)
{
	return false;
}

bool Layer2D::on_event(const WindowResizeEvent& event)
{
	camera_ctl_.on_window_resize_event(event);
	return false;
}

bool Layer2D::on_event(const MouseScrollEvent& event)
{
	camera_ctl_.on_mouse_scroll_event(event);
	return false;
}
