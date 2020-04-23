#include "layer_game.h"
#include "erwin.h"
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
	REFLECT_COMPONENT(ComponentMesh);
	REFLECT_COMPONENT(ComponentPBRMaterial);
	REFLECT_COMPONENT(ComponentDirectionalLightMaterial);
	REFLECT_COMPONENT(ComponentDirectionalLight);

	forward_skybox_render_system_.init();

	// TextureAtlasHandle atlas = AssetManager::load_texture_atlas("textures/atlas/set1.cat");

	const Material& mat_paved_floor = AssetManager::create_material<ComponentPBRMaterial>
	(
		"Paved floor",
		"shaders/deferred_PBR.glsl",
		"textures/map/pavedFloor.tom"
	);

	const Material& mat_rock = AssetManager::create_material<ComponentPBRMaterial>
	(
		"Rock tiling",
		"shaders/deferred_PBR.glsl",
		"textures/map/rockTiling.tom"
	);

	const Material& mat_dirt = AssetManager::create_material<ComponentPBRMaterial>
	(
		"Dirt",
		"shaders/deferred_PBR.glsl",
		"textures/map/dirt.tom"
	);

	const Material& mat_test_emissive = AssetManager::create_material<ComponentPBRMaterial>
	(
		"Magma",
		"shaders/deferred_PBR.glsl",
		"textures/map/testEmissive.tom"
	);

	const Material& mat_uniform = AssetManager::create_material<ComponentPBRMaterial>
	(
		"Unimat",
		"shaders/deferred_PBR.glsl"
	);

	const Material& mat_sun = AssetManager::create_material<ComponentDirectionalLightMaterial>
	(
		"Sun",
		"shaders/forward_sun.glsl"
	);

	{
		EntityID ent = Scene::registry.create();

		ComponentDirectionalLight cdirlight;
		cdirlight.set_position(47.626f, 49.027f);
		cdirlight.color         = {0.95f,0.85f,0.5f};
		cdirlight.ambient_color = {0.95f,0.85f,0.5f};
		cdirlight.ambient_strength = 0.1f;
		cdirlight.brightness = 3.7f;

		ComponentDirectionalLightMaterial renderable;
		renderable.set_material(mat_sun);
		renderable.material_data.scale = 0.2f;

		Scene::registry.assign<ComponentDirectionalLight>(ent, cdirlight);
		Scene::registry.assign<ComponentDirectionalLightMaterial>(ent, renderable);

		Scene::directional_light = ent;
		Scene::add_entity(ent, "Sun", W_ICON(SUN_O));
	}

	glm::vec3 pos[] = 
	{
		{-3.f,0.f,-1.f},
		{-1.f,0.f,-1.f},
		{ 1.f,0.f,-1.f},
		{ 3.f,0.f,-1.f},
	};
	std::array mats =
	{
		std::cref(mat_paved_floor),
		std::cref(mat_rock),
		std::cref(mat_dirt),
		std::cref(mat_test_emissive),
	};
	for(int ii=0; ii<4; ++ii)
	{
		EntityID ent = Scene::registry.create();

		ComponentTransform3D ctransform = {pos[ii], {0.f,0.f,0.f}, 1.8f};

		ComponentOBB cOBB(CommonGeometry::get_extent("cube_pbr"_h));
		cOBB.update(ctransform.get_model_matrix(), ctransform.uniform_scale);

		ComponentMesh cmesh;
		cmesh.set_vertex_array(CommonGeometry::get_vertex_array("cube_pbr"_h));

		ComponentPBRMaterial cmaterial;
		cmaterial.set_material(mats[ii]);
		cmaterial.enable_albedo_map();
		cmaterial.enable_normal_map();
		cmaterial.enable_metallic_map();
		cmaterial.enable_ao_map();
		cmaterial.enable_roughness_map();
		if(ii==3)
		{
			cmaterial.enable_emissivity();
			cmaterial.material_data.emissive_scale = 5.f;
		}

		cmaterial.material_data.tint = {1.f,1.f,1.f,1.f};

		Scene::registry.assign<ComponentTransform3D>(ent, ctransform);
		Scene::registry.assign<ComponentOBB>(ent, cOBB);
		Scene::registry.assign<ComponentMesh>(ent, cmesh);
		Scene::registry.assign<ComponentPBRMaterial>(ent, cmaterial);


		Scene::add_entity(ent, "Cube #" + std::to_string(ii));
	}


	constexpr int k_row = 5;
	for(int ii=0; ii<k_row; ++ii)
	{
		float metallic = float(ii)/float(k_row-1);
		for(int jj=0; jj<k_row; ++jj)
		{
			float roughness = float(jj)/float(k_row-1);
			roughness = std::max(roughness, 0.1f);

			EntityID ent = Scene::registry.create();

			ComponentTransform3D ctransform = {{2.5*ii,5.f+2.5*jj,8.f}, {0.f,0.f,0.f}, 0.75f};

			ComponentOBB cOBB(CommonGeometry::get_extent("icosphere_pbr"_h));
			cOBB.update(ctransform.get_model_matrix(), ctransform.uniform_scale);

			ComponentMesh cmesh;
			cmesh.set_vertex_array(CommonGeometry::get_vertex_array("icosphere_pbr"_h));

			ComponentPBRMaterial cmaterial;
			cmaterial.set_material(mat_uniform);
			cmaterial.material_data.uniform_metallic = metallic;
			cmaterial.material_data.uniform_roughness = roughness;
			cmaterial.material_data.uniform_albedo = {1.0f,1.0f,1.0f,1.f};

			Scene::registry.assign<ComponentTransform3D>(ent, ctransform);
			Scene::registry.assign<ComponentOBB>(ent, cOBB);
			Scene::registry.assign<ComponentMesh>(ent, cmesh);
			Scene::registry.assign<ComponentPBRMaterial>(ent, cmaterial);

			Scene::add_entity(ent, "Icosphere #" + std::to_string(ii*k_row+jj));
		}
	}



	Scene::camera_controller.set_position({-5.8f,2.3f,-5.8f});
	Scene::camera_controller.set_angles(228.f, 5.f);
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
	forward_skybox_render_system_.render();
	forward_sun_render_system_.render();

	// Presentation
	// TODO: do post processing here if editor is turned off
	// atm, EditorLayer is responsible for the post processing pass.
	// The idea is that the last layer should do it. Maybe this is
	// not well thought through...
	// PostProcessingRenderer::bloom_pass("LBuffer"_h, 1);
	// PostProcessingRenderer::combine("LBuffer"_h, 0, true);
	// // PostProcessingRenderer::combine("SpriteBuffer"_h, 0, false);
}

bool GameLayer::on_event(const MouseButtonEvent& event)
{
	return Scene::camera_controller.on_mouse_button_event(event);
}

bool GameLayer::on_event(const WindowResizeEvent& event)
{
	return Scene::camera_controller.on_window_resize_event(event);
}

bool GameLayer::on_event(const WindowMovedEvent& event)
{
	return false;
}

bool GameLayer::on_event(const MouseScrollEvent& event)
{
	return Scene::camera_controller.on_mouse_scroll_event(event);
}

bool GameLayer::on_event(const MouseMovedEvent& event)
{
	return Scene::camera_controller.on_mouse_moved_event(event);
}

bool GameLayer::on_event(const erwin::KeyboardEvent& event)
{	
	return Scene::camera_controller.on_keyboard_event(event);
}

