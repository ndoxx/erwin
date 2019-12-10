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
}

void Layer2D::on_detach()
{

}

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
	const auto& renderables = dungeon.get_renderables();

	Renderer2D::begin_pass(pass_state, camera_ctl_.get_camera(), get_layer_id());
	for(auto&& r: renderables)
	{
		auto& atlas = (r.atlas == "connectivity"_h) ? connectivity_atlas_ : dungeon_atlas_;
		if(r.has_tint)
			Renderer2D::draw_quad(r.position, r.scale, atlas.get_uv(r.tile), r.atlas, r.tint);
		else
			Renderer2D::draw_quad(r.position, r.scale, atlas.get_uv(r.tile), r.atlas);
	}
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
