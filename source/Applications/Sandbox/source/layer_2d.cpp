#include "layer_2d.h"

#include <iostream>
#include <iomanip>
#include <bitset>

#include "erwin.h"

#include "memory/memory_utils.h"

using namespace erwin;

Layer2D::Layer2D(): Layer("2DLayer"), camera_ctl_(1280.f/1024.f, 1.f)
{
    memory::hex_dump_highlight(0xf0f0f0f0, WCB(100,0,0));
    memory::hex_dump_highlight(0x0f0f0f0f, WCB(0,0,100));
    memory::hex_dump_highlight(0xd0d0d0d0, WCB(200,100,0));
}

void Layer2D::on_imgui_render()
{
	
}

void Layer2D::on_attach()
{
	atlas_ = AssetManager::load_texture_atlas("textures/atlas/set1.cat");
	arial_ = AssetManager::load_font_atlas("textures/atlas/arial.cat");

	// List of random sub-textures to use
	tiles_ =
	{
		"thatcha64"_h, "stonea64"_h, "pavingc64"_h, "tileroofa64"_h, "pavingd64"_h, "paneling64"_h, "rockc64"_h, "rocka64"_h,
		"thatchb64"_h, "sand64"_h, "planks64"_h, "rocke64"_h, "stonewalld64"_h, "pavinge64"_h, "stonec64"_h, "snow64"_h,
		"rockb64"_h, "stonewallb64"_h, "leaves64"_h, "rockd64"_h, "woodfloorb64"_h, "pavingf64"_h, "clover64"_h, "tileroofb64"_h,
		"grass64"_h, "dirt64"_h, "stoneb64"_h, "woodfloora64"_h, "stonewallc64"_h, "pavinga64"_h, "rockf64"_h, "pavingb64"_h,
		"stonewalla64"_h
	};
}

void Layer2D::on_detach()
{
	AssetManager::release(arial_);
	AssetManager::release(atlas_);
}

void Layer2D::on_update(GameClock& clock)
{
	float dt = clock.get_frame_duration();
	tt_ += dt;
	if(tt_>=5.f)
		tt_ = 0.f;

	camera_ctl_.update(clock);

	// Draw a grid of quads
	Renderer2D::begin_pass(camera_ctl_.get_camera(), true, get_layer_id());
	/*for(int xx=0; xx<len_grid_; ++xx)
	{
		float xx_offset = trippy_mode_ ? 3.0f/len_grid_ * cos(2*2*M_PI*xx/(1.f+len_grid_))*sin(0.2f*2*M_PI*tt_) : 0.f;
		float xx_scale = trippy_mode_ ? 1.0f/len_grid_ * (0.5f+sin(0.2f*2*M_PI*tt_)*sin(0.2f*2*M_PI*tt_)) : 1.f/float(len_grid_-1);
		float pos_x = -1.f + 2.f*xx/float(len_grid_-1) + xx_offset;

		for(int yy=0; yy<len_grid_; ++yy)
		{
			float yy_offset = trippy_mode_ ? 3.0f/len_grid_ * sin(2*2*M_PI*yy/(1.f+len_grid_))*cos(0.2f*2*M_PI*tt_) : 0.f;
			// float yy_scale = trippy_mode_ ? 1.0f/len_grid_ * (0.5f+sin(0.2f*2*M_PI*tt_)*sin(0.2f*2*M_PI*tt_)) : 1.f/float(len_grid_-1);
			float pos_y = -1.f + 2.f*yy/float(len_grid_-1) + yy_offset;

			// hash_t tile = tiles_.at((xx+yy)%(tiles_.size()-1));
			hash_t tile = tiles_.at((yy/3 + xx/5)%(tiles_.size()-1));
			Renderer2D::draw_quad(ComponentTransform2D({pos_x,pos_y,0.f},0.f,xx_scale), atlas_, tile);
		}
	}
	for(int xx=0; xx<5; ++xx)
	{
		float pos_x = -1.f + 2.f*xx/float(5-1);
		float xx_scale = 1.f/float(5-1);
		for(int yy=0; yy<5; ++yy)
		{
			float pos_y = -1.f + 2.f*yy/float(5-1);
			float yy_scale = 1.f/float(5-1);
			glm::vec4 color((xx+1)/6.f, (yy+1)/6.f, (xx+yy)/10.f, 1.f);
			Renderer2D::draw_colored_quad(ComponentTransform2D({pos_x,pos_y,-0.2f},0.f,xx_scale), color);
		}
	}*/
	Renderer2D::draw_text("Plip plop here is some text.", arial_, 0.1f, 0.1f, 20.f, {1.f,0.7f,0.f,1.f});
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
