#include "layer_2d.h"

#include <iostream>
#include <iomanip>
#include <bitset>

#include "erwin.h"

#include "memory/memory_utils.h"

using namespace erwin;

Layer2D::Layer2D(): Layer("2DLayer"), camera_ctl_(1280.f/1024.f, 1.f)
{

}

void Layer2D::on_imgui_render()
{

}

void Layer2D::on_attach()
{
	// atlas_.load("textures/atlas/set1.cat");
	// Renderer2D::register_atlas("set1"_h, atlas_);
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
