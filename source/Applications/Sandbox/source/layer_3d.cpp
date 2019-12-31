#include "layer_3d.h"

#include <iostream>
#include <iomanip>
#include <bitset>

#include "erwin.h"

#include "debug/texture_peek.h"
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
	TexturePeek::set_projection_parameters(camera_ctl_.get_camera().get_projection_parameters());
	MaterialLayoutHandle layout_a_nd_mra = AssetManager::create_material_layout({"albedo"_h, "normal_depth"_h, "mra"_h});
	forward_opaque_pbr_ = AssetManager::load_shader("shaders/forward_PBR.glsl");
	tg_ = AssetManager::load_texture_group("textures/map/sandstone.tom", layout_a_nd_mra);
	AssetManager::release(layout_a_nd_mra);

	// Setup scene
	for(float xx=-10.f; xx<10.f; xx+=2.f)
	{
		for(float yy=-10.f; yy<10.f; yy+=2.f)
		{
			for(float zz=-10.f; zz<10.f; zz+=2.f)
			{
				float scale = 3.f*sqrt(xx*xx+yy*yy+zz*zz)/sqrt(10*10*10);
				scene_.push_back(Cube{{{xx+1.f,yy+1.f,zz+1.f}, {0.f,0.f,0.f}, scale},
								      {forward_opaque_pbr_, tg_},
								      {(xx+10.f)/20.f,(yy+10.f)/20.f,(zz+10.f)/20.f,1.f}});
			}
		}
	}
}

void Layer3D::on_detach()
{
	AssetManager::release(tg_);
	AssetManager::release(forward_opaque_pbr_);
}

void Layer3D::on_update(GameClock& clock)
{
	float dt = clock.get_frame_duration();
	tt_ += dt;
	if(tt_>=10.f)
		tt_ = 0.f;

	camera_ctl_.update(clock);

	// Update scene
	for(Cube& cube: scene_)
	{
		float xx = cube.transform.position.x;
		float yy = cube.transform.position.y;
		float zz = cube.transform.position.z;
		glm::vec3 euler = {(1.f-yy/8.f)*xx/10.f,yy/10.f,(1.f-yy/8.f)*zz/10.f};
		euler *= 1.0f*sin(2*M_PI*tt_/10.f);
		cube.transform.set_rotation(euler);
	}

	// Draw scene
	VertexArrayHandle cube_pbr = CommonGeometry::get_vertex_array("cube_pbr"_h);

	ForwardRenderer::begin_pass(camera_ctl_.get_camera(), false, get_layer_id());
	for(auto&& cube: scene_)
		ForwardRenderer::draw_mesh(cube_pbr, cube.transform, cube.material, cube.tint);
	ForwardRenderer::end_pass();
}

bool Layer3D::on_event(const MouseButtonEvent& event)
{
	// BUG: Causes problems when screen center is within an ImGUI window
	// Cursor stays hidden
	ImGuiIO& io = ImGui::GetIO();
	if(!io.WantCaptureMouse) // TODO: Handle this automatically for all layers
		camera_ctl_.on_mouse_button_event(event);
	return false;
}

bool Layer3D::on_event(const WindowResizeEvent& event)
{
	camera_ctl_.on_window_resize_event(event);
	TexturePeek::set_projection_parameters(camera_ctl_.get_camera().get_projection_parameters());
	return false;
}

bool Layer3D::on_event(const MouseScrollEvent& event)
{
	ImGuiIO& io = ImGui::GetIO();
	if(!io.WantCaptureMouse) // TODO: Handle this automatically for all layers
		camera_ctl_.on_mouse_scroll_event(event);
	return false;
}

bool Layer3D::on_event(const MouseMovedEvent& event)
{
	ImGuiIO& io = ImGui::GetIO();
	if(!io.WantCaptureMouse) // TODO: Handle this automatically for all layers
		camera_ctl_.on_mouse_moved_event(event);
	return false;
}
