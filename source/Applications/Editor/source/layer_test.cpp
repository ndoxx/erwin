#include "layer_test.h"

#include <iostream>
#include <iomanip>
#include <bitset>

#include "erwin.h"

using namespace erwin;
using namespace editor;

LayerTest::LayerTest(Scene& scene): Layer("TestLayer"), scene_(scene)
{

}

void LayerTest::on_imgui_render()
{
	
}

void LayerTest::on_attach()
{
	MaterialLayoutHandle layout_a_nd_mare = AssetManager::create_material_layout({"albedo"_h, "normal_depth"_h, "mare"_h});
	deferred_pbr_       = AssetManager::load_shader("shaders/deferred_PBR.glsl");
	background_shader_  = AssetManager::load_shader("shaders/background.glsl");
	tg_                 = AssetManager::load_texture_group("textures/map/testEmissive.tom", layout_a_nd_mare);
	pbr_material_ubo_   = AssetManager::create_material_data_buffer(sizeof(PBRMaterialData));
	AssetManager::release(layout_a_nd_mare);

	DeferredRenderer::register_shader(deferred_pbr_, pbr_material_ubo_);

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

	scene_.camera_controller.set_position({0.f,1.f,3.f});

    pp_data_.set_flag_enabled(PP_EN_CHROMATIC_ABERRATION, true);
    pp_data_.set_flag_enabled(PP_EN_EXPOSURE_TONE_MAPPING, true);
    pp_data_.set_flag_enabled(PP_EN_VIBRANCE, true);
    pp_data_.set_flag_enabled(PP_EN_SATURATION, true);
    pp_data_.set_flag_enabled(PP_EN_CONTRAST, true);
    pp_data_.set_flag_enabled(PP_EN_GAMMA, true);
    pp_data_.set_flag_enabled(PP_EN_FXAA, true);

    PostProcessingRenderer::set_final_render_target("game_view"_h);
}

void LayerTest::on_detach()
{
	AssetManager::release(tg_);
	AssetManager::release(pbr_material_ubo_);
}

void LayerTest::on_update(GameClock& clock)
{
	float dt = clock.get_frame_duration();
	static float tt = 0.f;
	tt += dt;
	if(tt>=10.f)
		tt = 0.f;

	scene_.camera_controller.update(clock);

	float s = sin(2*M_PI*tt/10.f);
	float s2 = s*s;
	emissive_cube_.material_data.emissive_scale = 1.f + 5.f * exp(-8.f*s2);
	emissive_cube_.material_data.tint.r = 0.3f*exp(-12.f*s2);
}

void LayerTest::on_render()
{
	// FramebufferHandle fb = FramebufferPool::get_framebuffer("game_view"_h);
	// Renderer::clear(1, fb, ClearFlags::CLEAR_COLOR_FLAG, {1.0f,0.f,0.f,1.f});

	VertexArrayHandle cube_pbr = CommonGeometry::get_vertex_array("cube_pbr"_h);
	VertexArrayHandle quad     = CommonGeometry::get_vertex_array("quad"_h);

	// Draw scene geometry
	{
		DeferredRenderer::begin_pass(scene_.camera_controller.get_camera(), dir_light_);
		DeferredRenderer::draw_mesh(cube_pbr, emissive_cube_.transform, emissive_cube_.material);
		DeferredRenderer::end_pass();
	}

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

bool LayerTest::on_event(const MouseButtonEvent& event)
{
	ImGuiIO& io = ImGui::GetIO();
	if(!io.WantCaptureMouse)
		scene_.camera_controller.on_mouse_button_event(event);
	return false;
}

bool LayerTest::on_event(const WindowResizeEvent& event)
{
	scene_.camera_controller.on_window_resize_event(event);
	return false;
}

bool LayerTest::on_event(const WindowMovedEvent& event)
{
	scene_.camera_controller.on_window_moved_event(event);
	return false;
}

bool LayerTest::on_event(const MouseScrollEvent& event)
{
	ImGuiIO& io = ImGui::GetIO();
	if(!io.WantCaptureMouse)
		scene_.camera_controller.on_mouse_scroll_event(event);
	return false;
}

bool LayerTest::on_event(const MouseMovedEvent& event)
{
	/*ImGuiIO& io = ImGui::GetIO();
	if(!io.WantCaptureMouse)*/
		scene_.camera_controller.on_mouse_moved_event(event);
	return false;
}

bool LayerTest::on_event(const KeyboardEvent& event)
{
	ImGuiIO& io = ImGui::GetIO();
	if(!io.WantCaptureKeyboard)
		scene_.camera_controller.on_keyboard_event(event);
	return false;
}
