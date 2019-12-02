#include "layer_3d.h"

#include <iostream>
#include <iomanip>
#include <bitset>

#include "erwin.h"

#include "memory/memory_utils.h"
#include "glm/gtx/string_cast.hpp"

using namespace erwin;

Layer3D::Layer3D(): Layer("3DLayer"), camera_ctl_(1280.f/1024.f, 60, 0.1f, 100.f)
{

}

void Layer3D::on_imgui_render()
{

}

void Layer3D::on_attach()
{

}

void Layer3D::on_detach()
{

}

void Layer3D::on_update(GameClock& clock)
{
	float dt = clock.get_frame_duration();
	tt_ += dt;
	if(tt_>=5.f)
		tt_ = 0.f;

	camera_ctl_.update(clock);

	PassState pass_state;
	pass_state.rasterizer_state.cull_mode = CullMode::Back;
	pass_state.blend_state = BlendState::Opaque;
	pass_state.depth_stencil_state.depth_test_enabled = true;
	pass_state.rasterizer_state.clear_color = glm::vec4(0.2f,0.2f,0.2f,1.f);

	ForwardRenderer::begin_pass(pass_state, camera_ctl_.get_camera(), get_layer_id());
	for(float xx=-10.f; xx<=10.f; xx+=2.f)
	{
		for(float yy=-10.f; yy<=10.f; yy+=2.f)
		{
			for(float zz=-10.f; zz<=10.f; zz+=2.f)
			{
				float scale = 2.f*sqrt(xx*xx+yy*yy+zz*zz)/sqrt(10*10*10);
				ForwardRenderer::draw_colored_cube({xx,yy,zz}, scale, {fabs(xx/10.f),fabs(yy/10.f),fabs(zz/10.f),1.f});
			}
		}
	}
	ForwardRenderer::end_pass();
}

bool Layer3D::on_event(const MouseButtonEvent& event)
{
	return false;
}

bool Layer3D::on_event(const WindowResizeEvent& event)
{
	camera_ctl_.on_window_resize_event(event);
	return false;
}

bool Layer3D::on_event(const MouseScrollEvent& event)
{
	camera_ctl_.on_mouse_scroll_event(event);
	return false;
}

bool Layer3D::on_event(const MouseMovedEvent& event)
{
	camera_ctl_.on_mouse_moved_event(event);
	return false;
}
