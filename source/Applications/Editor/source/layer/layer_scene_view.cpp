#include "layer_scene_view.h"
#include "erwin.h"
#include "imgui/font_awesome.h"
#include "core/application.h"
#include "level/scene.h"

#include <iostream>
#include <iomanip>
#include <bitset>

using namespace erwin;

SceneViewLayer::SceneViewLayer():
Layer("SceneViewLayer")
{

}

void SceneViewLayer::on_imgui_render()
{
	
}

void SceneViewLayer::on_attach()
{	
	Scene::camera_controller.init(1280.f/1024.f, 60, 0.1f, 100.f);
	
	// Load resources
	// Scene::load_hdr_environment("textures/hdr/autumn_park_2k.hdr");
	// Scene::load_hdr_environment("textures/hdr/lakeside_1k.hdr");
	// Scene::load_hdr_environment("textures/hdr/dirt_bike_track_01_1k.hdr");
	// Scene::load_hdr_environment("textures/hdr/georgentor_2k.hdr");
	Scene::load_hdr_environment("textures/hdr/small_cathedral_2k.hdr");


	// TextureAtlasHandle atlas = AssetManager::load_texture_atlas("textures/atlas/set1.cat");

	ComponentPBRMaterial mat_greasy_metal = AssetManager::load_PBR_material(filesystem::get_asset_dir() / "textures/map/greasyMetal.tom");

	Material mat_sun;
	mat_sun.archetype = "Sun"_h;
	mat_sun.shader = AssetManager::load_shader("shaders/forward_sun.glsl");
	mat_sun.ubo = AssetManager::create_material_data_buffer<ComponentDirectionalLightMaterial>();
	mat_sun.data_size = sizeof(ComponentDirectionalLightMaterial::MaterialData);
	Renderer3D::register_shader(mat_sun.shader);
	Renderer::shader_attach_uniform_buffer(mat_sun.shader, mat_sun.ubo);

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

	{
		EntityID ent = Scene::registry.create();

		ComponentTransform3D ctransform = {{0.f,0.f,0.f}, {0.f,0.f,0.f}, 1.8f};

		ComponentOBB cOBB(CommonGeometry::get_extent("cube_pbr"_h));
		cOBB.update(ctransform.get_model_matrix(), ctransform.uniform_scale);

		ComponentMesh cmesh;
		cmesh.set_vertex_array(CommonGeometry::get_vertex_array("cube_pbr"_h));

		Scene::registry.assign<ComponentTransform3D>(ent, ctransform);
		Scene::registry.assign<ComponentOBB>(ent, cOBB);
		Scene::registry.assign<ComponentMesh>(ent, cmesh);
		Scene::registry.assign<ComponentPBRMaterial>(ent, mat_greasy_metal);


		Scene::add_entity(ent, "Cube #0");
	}

	Scene::camera_controller.set_position({-5.8f,2.3f,-5.8f});
	Scene::camera_controller.set_angles(228.f, 5.f);
}

void SceneViewLayer::on_detach()
{

}

void SceneViewLayer::on_update(GameClock& clock)
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

void SceneViewLayer::on_render()
{
	// FramebufferHandle fb = FramebufferPool::get_framebuffer("host"_h);
	// Renderer::clear(1, fb, ClearFlags::CLEAR_COLOR_FLAG, {1.0f,0.f,0.f,1.f});

	// Draw scene geometry
    {
        Renderer3D::begin_deferred_pass();
        auto view = Scene::registry.view<ComponentTransform3D, ComponentPBRMaterial, ComponentMesh>();
        for(const entt::entity e : view)
        {
            const ComponentTransform3D& ctransform = view.get<ComponentTransform3D>(e);
            ComponentPBRMaterial& cmaterial = view.get<ComponentPBRMaterial>(e);
            ComponentMesh& cmesh = view.get<ComponentMesh>(e);
            if(cmaterial.is_ready() && cmesh.is_ready())
                Renderer3D::draw_mesh(cmesh.vertex_array, ctransform.get_model_matrix(), cmaterial.material,
                                      &cmaterial.material_data);
        }
        Renderer3D::end_deferred_pass();
    }

    Renderer3D::draw_skybox(Scene::environment.environment_map);

    {
        VertexArrayHandle quad = CommonGeometry::get_vertex_array("quad"_h);

        Renderer3D::begin_forward_pass(BlendState::Light);
        auto view = Scene::registry.view<ComponentDirectionalLight, ComponentDirectionalLightMaterial>();
        for(const entt::entity e : view)
        {
            const ComponentDirectionalLight& dirlight = view.get<ComponentDirectionalLight>(e);
            ComponentDirectionalLightMaterial& renderable = view.get<ComponentDirectionalLightMaterial>(e);
            if(!renderable.is_ready())
                continue;

            renderable.material_data.color = glm::vec4(dirlight.color, 1.f);
            renderable.material_data.brightness = dirlight.brightness;

            Renderer3D::draw_mesh(quad, glm::mat4(1.f), renderable.material, &renderable.material_data);
        }
        Renderer3D::end_forward_pass();
    }
}

bool SceneViewLayer::on_event(const MouseButtonEvent& event)
{
	return Scene::camera_controller.on_mouse_button_event(event);
}

bool SceneViewLayer::on_event(const WindowResizeEvent& event)
{
	return Scene::camera_controller.on_window_resize_event(event);
}

bool SceneViewLayer::on_event(const WindowMovedEvent& event)
{
	return Scene::camera_controller.on_window_moved_event(event);
}

bool SceneViewLayer::on_event(const MouseScrollEvent& event)
{
	return Scene::camera_controller.on_mouse_scroll_event(event);
}

bool SceneViewLayer::on_event(const MouseMovedEvent& event)
{
	return Scene::camera_controller.on_mouse_moved_event(event);
}

bool SceneViewLayer::on_event(const erwin::KeyboardEvent& event)
{	
	return Scene::camera_controller.on_keyboard_event(event);
}

