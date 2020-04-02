#include "layer_game.h"
#include "erwin.h"
#include "game/game_components.h"
#include "entity/component_transform.h"
#include "entity/component_bounding_box.h"
#include "editor/font_awesome.h"
#include "core/application.h"

#include <iostream>
#include <iomanip>
#include <bitset>

using namespace erwin;

GameLayer::GameLayer():
Layer("GameLayer")
{

}

void GameLayer::on_imgui_render()
{
	
}

void GameLayer::on_attach()
{	
	REFLECT_COMPONENT(ComponentTransform3D);
	REFLECT_COMPONENT(ComponentOBB);
	REFLECT_COMPONENT(ComponentRenderablePBR);
	REFLECT_COMPONENT(ComponentRenderableDirectionalLight);
	REFLECT_COMPONENT(ComponentDirectionalLight);

	MaterialLayoutHandle layout_a_nd_mare = AssetManager::create_material_layout({"albedo"_h, "normal_depth"_h, "mare"_h});
	ShaderHandle forward_sun              = AssetManager::load_shader("shaders/forward_sun.glsl");
	ShaderHandle deferred_pbr             = AssetManager::load_shader("shaders/deferred_PBR.glsl");
	TextureGroupHandle tg                 = AssetManager::load_texture_group("textures/map/testEmissive.tom", layout_a_nd_mare);
	UniformBufferHandle pbr_material_ubo  = AssetManager::create_material_data_buffer(sizeof(ComponentRenderablePBR::MaterialData));
	UniformBufferHandle sun_material_ubo  = AssetManager::create_material_data_buffer(sizeof(ComponentRenderableDirectionalLight::MaterialData));
	AssetManager::release(layout_a_nd_mare);
	Renderer3D::register_material({deferred_pbr, {}, pbr_material_ubo});
	Renderer3D::register_material({forward_sun, {}, sun_material_ubo});

	{
		EntityID ent = Scene::registry.create();

		ComponentDirectionalLight directional_light;
		directional_light.set_position(90.f, 160.f);
		directional_light.color         = {0.95f,0.85f,0.5f};
		directional_light.ambient_color = {0.95f,0.85f,0.5f};
		directional_light.ambient_strength = 0.1f;
		directional_light.brightness = 3.7f;

		ComponentRenderableDirectionalLight renderable;
		renderable.material.shader = forward_sun;
		renderable.material.ubo = sun_material_ubo;
		renderable.material_data.scale = 0.2f;

		Scene::registry.assign<ComponentDirectionalLight>(ent, directional_light);
		Scene::registry.assign<ComponentRenderableDirectionalLight>(ent, renderable);

		Scene::directional_light = ent;
		Scene::add_entity(ent, "Sun", ICON_FA_SUN_O);
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
		EntityID ent = Scene::registry.create();

		ComponentTransform3D transform = {pos[ii], {0.f,0.f,0.f}, 1.8f};

		ComponentOBB OBB(CommonGeometry::get_extent("cube_pbr"_h));
		OBB.update(transform.get_model_matrix(), transform.uniform_scale);

		ComponentRenderablePBR renderable;
		renderable.vertex_array = CommonGeometry::get_vertex_array("cube_pbr"_h);
		renderable.set_emissive(5.f);
		renderable.material.shader = deferred_pbr;
		renderable.material.texture_group = tg;
		renderable.material.ubo = pbr_material_ubo;
		renderable.material_data.tint = {1.f,1.f,1.f,1.f};

		Scene::registry.assign<ComponentTransform3D>(ent, transform);
		Scene::registry.assign<ComponentOBB>(ent, OBB);
		Scene::registry.assign<ComponentRenderablePBR>(ent, renderable);


		Scene::add_entity(ent, "Emissive cube #" + std::to_string(ii));
	}

	Scene::camera_controller.set_position({0.f,1.f,3.f});
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

    Scene::camera_controller.update(clock);
	bounding_box_system_.update(clock);

	// TMP: Update cube -> MOVE to script or animation system
	// for(int ii=0; ii<4; ++ii)
	// {
	// 	float s = sin(2*M_PI*tt/10.f + M_PI*0.25f*ii);
	// 	float s2 = s*s;

	// 	auto& renderable = Scene::registry.get<ComponentRenderablePBR>(Scene::entities[1+ii]);

	// 	renderable.material_data.emissive_scale = 1.f + 5.f * exp(-4.f*(ii+1.f)*s2);
	// 	renderable.material_data.tint.r = 0.3f*exp(-6.f*(ii+1.f)*s2);
	// }

    // TMP: SCENE must have a directional light entity or this fails
    const auto& dirlight = Scene::registry.get<ComponentDirectionalLight>(Scene::directional_light);
    Renderer3D::update_frame_data(Scene::camera_controller.get_camera(), dirlight);
}

void GameLayer::on_render()
{
	// FramebufferHandle fb = FramebufferPool::get_framebuffer("game_view"_h);
	// Renderer::clear(1, fb, ClearFlags::CLEAR_COLOR_FLAG, {1.0f,0.f,0.f,1.f});

	// Draw scene geometry
	PBR_deferred_render_system_.render();
	forward_sun_render_system_.render();
	gizmo_system_.render();
	bounding_box_system_.render();

	// Presentation
	PostProcessingRenderer::bloom_pass("LBuffer"_h, 1);
	PostProcessingRenderer::combine("LBuffer"_h, 0, true);
	// PostProcessingRenderer::combine("SpriteBuffer"_h, 0, false);
}

bool GameLayer::on_event(const MouseButtonEvent& event)
{
	return false;
}

bool GameLayer::on_event(const WindowResizeEvent& event)
{
	return false;
}

bool GameLayer::on_event(const WindowMovedEvent& event)
{
	return false;
}

bool GameLayer::on_event(const MouseScrollEvent& event)
{
	return false;
}

bool GameLayer::on_event(const MouseMovedEvent& event)
{
	return false;
}
