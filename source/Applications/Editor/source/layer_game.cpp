#include "layer_game.h"
#include "erwin.h"
#include "game/game_components.h"
#include "game/pbr_deferred_render_system.h"
#include "game/forward_sun_render_system.h"
#include "game/debug_render_system.h"
#include "game/bounding_box_system.h"
#include "entity/component_transform.h"
#include "entity/component_bounding_box.h"
#include "editor/font_awesome.h"

#include <iostream>
#include <iomanip>
#include <bitset>

using namespace erwin;

GameLayer::GameLayer(erwin::Scene& scene, EntityManager& emgr, memory::HeapArea& client_area):
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
	entity_manager_.create_component_manager<ComponentOBB>(client_area_, 128);
	entity_manager_.create_component_manager<ComponentRenderablePBR>(client_area_, 128);
	entity_manager_.create_component_manager<ComponentRenderableDirectionalLight>(client_area_, 2);
	entity_manager_.create_component_manager<ComponentDirectionalLight>(client_area_, 2);

	entity_manager_.create_system<BoundingBoxSystem>();
	auto* pbr_deferred_render_system = entity_manager_.create_system<PBRDeferredRenderSystem>();
	pbr_deferred_render_system->set_scene(&scene_);
	auto* forward_sun_render_system = entity_manager_.create_system<ForwardSunRenderSystem>();
	forward_sun_render_system->set_scene(&scene_);
	auto* debug_render_system = entity_manager_.create_system<DebugRenderSystem>();
	debug_render_system->set_scene(&scene_);

	MaterialLayoutHandle layout_a_nd_mare = AssetManager::create_material_layout({"albedo"_h, "normal_depth"_h, "mare"_h});
	ShaderHandle forward_sun              = AssetManager::load_shader("shaders/forward_sun.glsl");
	ShaderHandle deferred_pbr             = AssetManager::load_shader("shaders/deferred_PBR.glsl");
	TextureGroupHandle tg                 = AssetManager::load_texture_group("textures/map/testEmissive.tom", layout_a_nd_mare);
	UniformBufferHandle pbr_material_ubo  = AssetManager::create_material_data_buffer(sizeof(ComponentRenderablePBR::MaterialData));
	UniformBufferHandle sun_material_ubo  = AssetManager::create_material_data_buffer(sizeof(ComponentRenderableDirectionalLight::MaterialData));
	AssetManager::release(layout_a_nd_mare);
	DeferredRenderer::register_shader(deferred_pbr, pbr_material_ubo);
	ForwardRenderer::register_shader(forward_sun, sun_material_ubo);

	{
		EntityID ent = entity_manager_.create_entity();
		auto& directional_light = entity_manager_.create_component<ComponentDirectionalLight>(ent);
		auto& renderable        = entity_manager_.create_component<ComponentRenderableDirectionalLight>(ent);

		directional_light.set_position(90.f, 160.f);
		directional_light.color         = {0.95f,0.85f,0.5f};
		directional_light.ambient_color = {0.95f,0.85f,0.5f};
		directional_light.ambient_strength = 0.1f;
		directional_light.brightness = 3.7f;

		renderable.material.shader = forward_sun;
		renderable.material.ubo = sun_material_ubo;
		renderable.material_data.scale = 0.2f;
		entity_manager_.submit_entity(ent);
		scene_.directional_light = ent;
		scene_.add_entity(ent, "Sun", ICON_FA_SUN_O);
	}

	glm::vec3 pos[] = 
	{
		{-1.f,0.f,-1.f},
		{ 1.f,0.f,-1.f},
		{-1.f,0.f, 1.f},
		{ 1.f,0.f, 1.f},
	};
	for(int ii=0; ii<4; ++ii)
	{
		EntityID ent     = entity_manager_.create_entity();
		auto& transform  = entity_manager_.create_component<ComponentTransform3D>(ent);
		auto& renderable = entity_manager_.create_component<ComponentRenderablePBR>(ent);
		auto& OBB        = entity_manager_.create_component<ComponentOBB>(ent);
		transform = {pos[ii], {0.f,0.f,0.f}, 1.8f};
		OBB.init(CommonGeometry::get_extent("cube_pbr"_h));
		OBB.update(transform.get_model_matrix(), transform.uniform_scale);
		renderable.vertex_array = CommonGeometry::get_vertex_array("cube_pbr"_h);
		renderable.set_emissive(5.f);
		renderable.material.shader = deferred_pbr;
		renderable.material.texture_group = tg;
		renderable.material.ubo = pbr_material_ubo;
		renderable.material_data.tint = {0.f,1.f,1.f,1.f};
		entity_manager_.submit_entity(ent);
		scene_.add_entity(ent, "Emissive cube #" + std::to_string(ii));
	}

	scene_.selected_entity_idx = 0;

	scene_.camera_controller.set_position({0.f,1.f,3.f});

    scene_.post_processing.set_flag_enabled(PP_EN_CHROMATIC_ABERRATION, true);
    scene_.post_processing.set_flag_enabled(PP_EN_EXPOSURE_TONE_MAPPING, true);
    scene_.post_processing.set_flag_enabled(PP_EN_VIBRANCE, true);
    scene_.post_processing.set_flag_enabled(PP_EN_SATURATION, true);
    scene_.post_processing.set_flag_enabled(PP_EN_CONTRAST, true);
    scene_.post_processing.set_flag_enabled(PP_EN_GAMMA, true);
    scene_.post_processing.set_flag_enabled(PP_EN_FXAA, true);
}

void GameLayer::on_detach()
{

}

void GameLayer::on_update(GameClock& clock)
{
	float dt = clock.get_frame_duration();
	static float tt = 0.f;
	tt += dt;
	if(tt>=10.f)
		tt = 0.f;

	entity_manager_.update(clock);

	scene_.camera_controller.update(clock);

	// TMP: Update cube -> MOVE to Lua script
	for(int ii=0; ii<4; ++ii)
	{
		float s = sin(2*M_PI*tt/10.f + M_PI*0.25f*ii);
		float s2 = s*s;

		Entity& cube = entity_manager_.get_entity(scene_.entities[1+ii].id);
		auto* renderable = cube.get_component<ComponentRenderablePBR>();

		renderable->material_data.emissive_scale = 1.f + 5.f * exp(-4.f*(ii+1.f)*s2);
		renderable->material_data.tint.r = 0.3f*exp(-6.f*(ii+1.f)*s2);
	}
}

void GameLayer::on_render()
{
	// FramebufferHandle fb = FramebufferPool::get_framebuffer("game_view"_h);
	// Renderer::clear(1, fb, ClearFlags::CLEAR_COLOR_FLAG, {1.0f,0.f,0.f,1.f});

	// Draw scene geometry
	entity_manager_.render();

	// Presentation
	PostProcessingRenderer::bloom_pass("LBuffer"_h, 1);
	scene_.post_processing.set_flag_enabled(PP_EN_BLOOM, true);
	PostProcessingRenderer::combine("LBuffer"_h, 0, scene_.post_processing);
	// scene_.post_processing.clear_flag(PP_EN_BLOOM);
	// PostProcessingRenderer::combine("SpriteBuffer"_h, 0, scene_.post_processing);
}

bool GameLayer::on_event(const MouseButtonEvent& event)
{
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
	scene_.camera_controller.on_mouse_scroll_event(event);
	return false;
}

bool GameLayer::on_event(const MouseMovedEvent& event)
{
	scene_.camera_controller.on_mouse_moved_event(event);
	return false;
}
