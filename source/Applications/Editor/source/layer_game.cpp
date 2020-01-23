#include "layer_game.h"
#include "erwin.h"
#include "game/game_components.h"
#include "entity/component_transform.h"

#include <iostream>
#include <iomanip>
#include <bitset>

using namespace erwin;
using namespace game;

GameLayer::GameLayer(game::Scene& scene, EntityManager& emgr, memory::HeapArea& client_area):
Layer("GameLayer"),
scene_(scene),
entity_manager_(emgr),
client_area_(client_area)
{

}

void GameLayer::on_imgui_render()
{
	
}

void GameLayer::on_attach()
{	
	entity_manager_.create_component_manager<ComponentTransform3D>(client_area_, 128);
	entity_manager_.create_component_manager<ComponentRenderablePBRDeferred>(client_area_, 128);


	scene_.init(entity_manager_);

    scene_.post_processing.set_flag_enabled(PP_EN_CHROMATIC_ABERRATION, true);
    scene_.post_processing.set_flag_enabled(PP_EN_EXPOSURE_TONE_MAPPING, true);
    scene_.post_processing.set_flag_enabled(PP_EN_VIBRANCE, true);
    scene_.post_processing.set_flag_enabled(PP_EN_SATURATION, true);
    scene_.post_processing.set_flag_enabled(PP_EN_CONTRAST, true);
    scene_.post_processing.set_flag_enabled(PP_EN_GAMMA, true);
    scene_.post_processing.set_flag_enabled(PP_EN_FXAA, true);

    PostProcessingRenderer::set_final_render_target("game_view"_h);
}

void GameLayer::on_detach()
{
	scene_.shutdown();
}

void GameLayer::on_update(GameClock& clock)
{
	float dt = clock.get_frame_duration();
	static float tt = 0.f;
	tt += dt;
	if(tt>=10.f)
		tt = 0.f;

	scene_.update(clock);

	// Update cube
	Entity& cube = entity_manager_.get_entity(scene_.cube_ent);
	auto* renderable = cube.get_component<ComponentRenderablePBRDeferred>();

	float s = sin(2*M_PI*tt/10.f);
	float s2 = s*s;
	renderable->material_data.emissive_scale = 1.f + 5.f * exp(-8.f*s2);
	renderable->material_data.tint.r = 0.3f*exp(-12.f*s2);
}

void GameLayer::on_render()
{
	// FramebufferHandle fb = FramebufferPool::get_framebuffer("game_view"_h);
	// Renderer::clear(1, fb, ClearFlags::CLEAR_COLOR_FLAG, {1.0f,0.f,0.f,1.f});

	VertexArrayHandle cube_pbr = CommonGeometry::get_vertex_array("cube_pbr"_h);
	VertexArrayHandle quad = CommonGeometry::get_vertex_array("quad"_h);

	// Draw scene geometry
	{
		DeferredRenderer::begin_pass(scene_.camera_controller.get_camera(), scene_.directional_light);
		Entity& cube = entity_manager_.get_entity(scene_.cube_ent);
		const auto* transform  = cube.get_component<ComponentTransform3D>();
		const auto* renderable = cube.get_component<ComponentRenderablePBRDeferred>();
		DeferredRenderer::draw_mesh(renderable->vertex_array, *transform, renderable->material);
		DeferredRenderer::end_pass();
	}

	// Draw sun
	{
		PassOptions options;
		options.set_transparency(true);
		options.set_depth_control(PassOptions::DEPTH_CONTROL_FAR);

		ForwardRenderer::begin_pass(scene_.camera_controller.get_camera(), scene_.directional_light, options);
		ForwardRenderer::draw_mesh(quad, ComponentTransform3D(), scene_.sun_material_);
		ForwardRenderer::end_pass();
	}

	// Presentation
	PostProcessingRenderer::bloom_pass("LBuffer"_h, 1);
	scene_.post_processing.set_flag_enabled(PP_EN_BLOOM, true);
	PostProcessingRenderer::combine("LBuffer"_h, 0, scene_.post_processing);
	// scene_.post_processing.clear_flag(PP_EN_BLOOM);
	// PostProcessingRenderer::combine("SpriteBuffer"_h, 0, scene_.post_processing);
}

bool GameLayer::on_event(const MouseButtonEvent& event)
{
	ImGuiIO& io = ImGui::GetIO();
	if(!io.WantCaptureMouse)
		scene_.camera_controller.on_mouse_button_event(event);
	return false;
}

bool GameLayer::on_event(const WindowResizeEvent& event)
{
	scene_.camera_controller.on_window_resize_event(event);
	return false;
}

bool GameLayer::on_event(const WindowMovedEvent& event)
{
	scene_.camera_controller.on_window_moved_event(event);
	return false;
}

bool GameLayer::on_event(const MouseScrollEvent& event)
{
	ImGuiIO& io = ImGui::GetIO();
	if(!io.WantCaptureMouse)
		scene_.camera_controller.on_mouse_scroll_event(event);
	return false;
}

bool GameLayer::on_event(const MouseMovedEvent& event)
{
	/*ImGuiIO& io = ImGui::GetIO();
	if(!io.WantCaptureMouse)*/
		scene_.camera_controller.on_mouse_moved_event(event);
	return false;
}
