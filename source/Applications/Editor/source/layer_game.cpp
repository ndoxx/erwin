#include "layer_game.h"

#include <iostream>
#include <iomanip>
#include <bitset>

#include "erwin.h"

using namespace erwin;
using namespace editor;

GameLayer::GameLayer(Scene& scene): Layer("GameLayer"), scene_(scene)
{

}

void GameLayer::on_imgui_render()
{
	
}

void GameLayer::on_attach()
{	
	scene_.init();

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
	scene_.update(clock);
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
		DeferredRenderer::draw_mesh(cube_pbr, scene_.emissive_cube.transform, scene_.emissive_cube.material);
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
