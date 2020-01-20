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
	background_shader_ = AssetManager::load_shader("shaders/background.glsl");
	
	scene_.init();

    pp_data_.set_flag_enabled(PP_EN_CHROMATIC_ABERRATION, true);
    pp_data_.set_flag_enabled(PP_EN_EXPOSURE_TONE_MAPPING, true);
    pp_data_.set_flag_enabled(PP_EN_VIBRANCE, true);
    pp_data_.set_flag_enabled(PP_EN_SATURATION, true);
    pp_data_.set_flag_enabled(PP_EN_CONTRAST, true);
    pp_data_.set_flag_enabled(PP_EN_GAMMA, true);
    pp_data_.set_flag_enabled(PP_EN_FXAA, true);

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
	VertexArrayHandle quad     = CommonGeometry::get_vertex_array("quad"_h);

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
	pp_data_.set_flag_enabled(PP_EN_BLOOM, true);
	PostProcessingRenderer::combine("LBuffer"_h, 0, pp_data_);
	// pp_data_.clear_flag(PP_EN_BLOOM);
	// PostProcessingRenderer::combine("SpriteBuffer"_h, 0, pp_data_);

	// WTF: we must draw something to the default framebuffer or else, whole screen is blank
	RenderState state;
	state.render_target = Renderer::default_render_target();
	state.rasterizer_state.cull_mode = CullMode::Back;
	state.rasterizer_state.clear_flags = CLEAR_COLOR_FLAG | CLEAR_DEPTH_FLAG;
	state.blend_state = BlendState::Opaque;
	state.depth_stencil_state.depth_test_enabled = true;

	SortKey key;
	key.set_sequence(0, Renderer::next_layer_id(), background_shader_);
	DrawCall dc(DrawCall::Indexed, state.encode(), background_shader_, quad);
	Renderer::submit(key.encode(), dc);
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
