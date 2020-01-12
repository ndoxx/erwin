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
	MaterialLayoutHandle layout_a_nd_mar  = AssetManager::create_material_layout({"albedo"_h, "normal_depth"_h, "mar"_h});
	MaterialLayoutHandle layout_a_nd_mare = AssetManager::create_material_layout({"albedo"_h, "normal_depth"_h, "mare"_h});
	forward_opaque_pbr_ = AssetManager::load_shader("shaders/forward_PBR.glsl");
	forward_sun_        = AssetManager::load_shader("shaders/forward_sun.glsl");
	tg_0_               = AssetManager::load_texture_group("textures/map/sandstone.tom", layout_a_nd_mar);
	tg_1_               = AssetManager::load_texture_group("textures/map/beachSand.tom", layout_a_nd_mar);
	tg_2_               = AssetManager::load_texture_group("textures/map/testEmissive.tom", layout_a_nd_mare);
	pbr_material_ubo_   = AssetManager::create_material_data_buffer(sizeof(PBRMaterialData));
	sun_material_ubo_   = AssetManager::create_material_data_buffer(sizeof(SunMaterialData));
	AssetManager::release(layout_a_nd_mar);
	AssetManager::release(layout_a_nd_mare);

	ForwardRenderer::register_shader(forward_opaque_pbr_, pbr_material_ubo_);
	ForwardRenderer::register_shader(forward_sun_, sun_material_ubo_);

	// Setup scene
	for(float xx=-10.f; xx<10.f; xx+=2.f)
	{
		for(float yy=-10.f; yy<10.f; yy+=2.f)
		{
			for(float zz=-10.f; zz<10.f; zz+=2.f)
			{
				float scale = 3.f*sqrt(xx*xx+yy*yy+zz*zz)/sqrt(10*10*10);
				Cube cube;
				cube.transform = {{xx,yy,zz}, {0.f,0.f,0.f}, scale};
				if(fabs(xx+1.f)>5.f || fabs(zz+1.f)>5.f)
					cube.material = {forward_opaque_pbr_, tg_0_, pbr_material_ubo_, nullptr, sizeof(PBRMaterialData)};
				else
					cube.material = {forward_opaque_pbr_, tg_1_, pbr_material_ubo_, nullptr, sizeof(PBRMaterialData)};
				cube.material_data.tint = {(xx+10.f)/20.f,(yy+10.f)/20.f,(zz+10.f)/20.f,1.f};
				scene_.push_back(cube);
			}
		}
	}

	Cube emissive_cube;
	emissive_cube.transform = {{0.f,0.f,0.f}, {0.f,0.f,0.f}, 2.f};
	emissive_cube.material = {forward_opaque_pbr_, tg_2_, pbr_material_ubo_, nullptr, sizeof(PBRMaterialData)};
	emissive_cube.material_data.tint = {1.f,1.f,1.f,1.f};
	emissive_cube.material_data.enable_emissivity();
	emissive_cube.material_data.emissive_scale = 5.f;
	scene_.push_back(emissive_cube);

	// I must setup all data pointers when I'm sure data won't move in memory due to vector realloc
	// TMP: this is awkward
	for(Cube& cube: scene_)
		cube.material.data = &cube.material_data;

	dir_light_.set_position(90.f, 160.f);
	dir_light_.color         = {0.95f,0.85f,0.5f};
	dir_light_.ambient_color = {0.95f,0.85f,0.5f};
	dir_light_.ambient_strength = 0.1f;
	dir_light_.brightness = 3.7f;


	// Setup Sun
	sun_material_.shader = forward_sun_;
	sun_material_.ubo = sun_material_ubo_;
	sun_material_.data = &sun_material_data_;
	sun_material_.data_size = sizeof(SunMaterialData);

	sun_material_data_.scale = 0.2f;
}

void Layer3D::on_detach()
{
	AssetManager::release(tg_0_);
	AssetManager::release(tg_1_);
	AssetManager::release(tg_2_);
	AssetManager::release(forward_opaque_pbr_);
	AssetManager::release(pbr_material_ubo_);
}

void Layer3D::on_update(GameClock& clock)
{
	float dt = clock.get_frame_duration();
	static float tt = 0.f;
	tt += dt;
	if(tt>=10.f)
		tt = 0.f;

	camera_ctl_.update(clock);

	// Update sun
	sun_material_data_.color = glm::vec4(dir_light_.color, 1.f);
	sun_material_data_.brightness = dir_light_.brightness;

	// Update scene
	for(Cube& cube: scene_)
	{
		float xx = cube.transform.position.x;
		float yy = cube.transform.position.y;
		float zz = cube.transform.position.z;
		glm::vec3 euler = {(1.f-yy/8.f)*xx/10.f,yy/10.f,(1.f-yy/8.f)*zz/10.f};
		euler *= 1.0f*sin(2*M_PI*tt/10.f);
		cube.transform.set_rotation(euler);
	}
}

void Layer3D::on_render()
{
	VertexArrayHandle quad     = CommonGeometry::get_vertex_array("quad"_h);
	VertexArrayHandle cube_uv  = CommonGeometry::get_vertex_array("cube_uv"_h);
	VertexArrayHandle cube_pbr = CommonGeometry::get_vertex_array("cube_pbr"_h);

	// Draw sun
	{
		PassOptions options;
		options.set_transparency(true);
		options.set_layer_id(get_layer_id());
		options.set_depth_control(PassOptions::DEPTH_CONTROL_FAR);

		ForwardRenderer::begin_pass(camera_ctl_.get_camera(), dir_light_, options);
		ForwardRenderer::draw_mesh(quad, ComponentTransform3D(), sun_material_);
		ForwardRenderer::end_pass();
	}

	// Draw scene geometry
	{
		PassOptions options;
		options.set_transparency(false);
		options.set_layer_id(get_layer_id());

		ForwardRenderer::begin_pass(camera_ctl_.get_camera(), dir_light_, options);
		for(auto&& cube: scene_)
			ForwardRenderer::draw_mesh(cube_pbr, cube.transform, cube.material);
		ForwardRenderer::end_pass();
	}
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
