#include "layer_3d_deferred.h"

#include <iostream>
#include <iomanip>
#include <bitset>

#include "erwin.h"

#include "debug/texture_peek.h"
#include "memory/memory_utils.h"
#include "glm/gtx/string_cast.hpp"

using namespace erwin;

Layer3DDeferred::Layer3DDeferred(): Layer("3DLayer"), camera_ctl_(1280.f/1024.f, 60, 0.1f, 100.f)
{

}

void Layer3DDeferred::on_imgui_render()
{

}

void Layer3DDeferred::on_attach()
{
	TexturePeek::set_projection_parameters(camera_ctl_.get_camera().get_projection_parameters());
	MaterialLayoutHandle layout_a_nd_mare = AssetManager::create_material_layout({"albedo"_h, "normal_depth"_h, "mare"_h});
	deferred_pbr_     = AssetManager::load_shader("shaders/deferred_PBR.glsl");
	tg_               = AssetManager::load_texture_group("textures/map/testEmissive.tom", layout_a_nd_mare);
	pbr_material_ubo_ = AssetManager::create_material_data_buffer(sizeof(PBRMaterialData));
	AssetManager::release(layout_a_nd_mare);

	DeferredRenderer::register_shader(deferred_pbr_, pbr_material_ubo_);

	// Setup scene
	emissive_cube_.transform = {{0.f,0.f,0.f}, {0.f,0.f,0.f}, 1.8f};
	emissive_cube_.material = {deferred_pbr_, tg_, pbr_material_ubo_, nullptr, sizeof(PBRMaterialData)};
	emissive_cube_.material_data.tint = {0.f,1.f,1.f,1.f};
	emissive_cube_.material_data.enable_emissivity();
	emissive_cube_.material_data.emissive_scale = 5.f;
	emissive_cube_.material.data = &emissive_cube_.material_data;

	dir_light_.set_position(90.f, 160.f);
	dir_light_.color         = {0.95f,0.85f,0.5f};
	dir_light_.ambient_color = {0.95f,0.85f,0.5f};
	dir_light_.ambient_strength = 0.1f;
	dir_light_.brightness = 3.7f;
}

void Layer3DDeferred::on_detach()
{
	AssetManager::release(tg_);
	AssetManager::release(deferred_pbr_);
	AssetManager::release(pbr_material_ubo_);
}

void Layer3DDeferred::on_update(GameClock& clock)
{
	float dt = clock.get_frame_duration();
	static float tt = 0.f;
	tt += dt;
	if(tt>=10.f)
		tt = 0.f;

	camera_ctl_.update(clock);

	// Update scene
	float xx = emissive_cube_.transform.position.x;
	float yy = emissive_cube_.transform.position.y;
	float zz = emissive_cube_.transform.position.z;
	glm::vec3 euler = {(1.f-yy/9.f)*xx/10.f,yy/10.f,(1.f-yy/9.f)*zz/10.f};
	euler *= 1.0f*sin(2*M_PI*tt/10.f);
	emissive_cube_.transform.set_rotation(euler);
}

void Layer3DDeferred::on_render()
{
	VertexArrayHandle cube_pbr = CommonGeometry::get_vertex_array("cube_pbr"_h);

	// Draw scene geometry
	{
		DeferredRenderer::begin_geometry_pass(camera_ctl_.get_camera(), get_layer_id());
		DeferredRenderer::draw_mesh(cube_pbr, emissive_cube_.transform, emissive_cube_.material);
		DeferredRenderer::end_geometry_pass();
	}
}

bool Layer3DDeferred::on_event(const MouseButtonEvent& event)
{
	// BUG: Causes problems when screen center is within an ImGUI window
	// Cursor stays hidden
	ImGuiIO& io = ImGui::GetIO();
	if(!io.WantCaptureMouse) // TODO: Handle this automatically for all layers
		camera_ctl_.on_mouse_button_event(event);
	return false;
}

bool Layer3DDeferred::on_event(const WindowResizeEvent& event)
{
	camera_ctl_.on_window_resize_event(event);
	TexturePeek::set_projection_parameters(camera_ctl_.get_camera().get_projection_parameters());
	return false;
}

bool Layer3DDeferred::on_event(const MouseScrollEvent& event)
{
	ImGuiIO& io = ImGui::GetIO();
	if(!io.WantCaptureMouse) // TODO: Handle this automatically for all layers
		camera_ctl_.on_mouse_scroll_event(event);
	return false;
}

bool Layer3DDeferred::on_event(const MouseMovedEvent& event)
{
	ImGuiIO& io = ImGui::GetIO();
	if(!io.WantCaptureMouse) // TODO: Handle this automatically for all layers
		camera_ctl_.on_mouse_moved_event(event);
	return false;
}
