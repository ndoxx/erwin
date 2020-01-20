#include "scene.h"

using namespace erwin;

namespace editor
{

Scene::Scene():
camera_controller(1280.f/1024.f, 60, 0.1f, 100.f)
{

}

void Scene::init()
{
	MaterialLayoutHandle layout_a_nd_mare = AssetManager::create_material_layout({"albedo"_h, "normal_depth"_h, "mare"_h});
	forward_sun_        = AssetManager::load_shader("shaders/forward_sun.glsl");
	deferred_pbr_       = AssetManager::load_shader("shaders/deferred_PBR.glsl");
	tg_                 = AssetManager::load_texture_group("textures/map/testEmissive.tom", layout_a_nd_mare);
	sun_material_ubo_   = AssetManager::create_material_data_buffer(sizeof(SunMaterialData));
	pbr_material_ubo_   = AssetManager::create_material_data_buffer(sizeof(PBRMaterialData));
	AssetManager::release(layout_a_nd_mare);

	ForwardRenderer::register_shader(forward_sun_, sun_material_ubo_);
	DeferredRenderer::register_shader(deferred_pbr_, pbr_material_ubo_);

	emissive_cube.transform = {{0.f,0.f,0.f}, {0.f,0.f,0.f}, 1.8f};
	emissive_cube.material = {deferred_pbr_, tg_, pbr_material_ubo_, nullptr, sizeof(PBRMaterialData)};
	emissive_cube.material_data.tint = {0.f,1.f,1.f,1.f};
	emissive_cube.material_data.enable_emissivity();
	emissive_cube.material_data.emissive_scale = 5.f;
	emissive_cube.material.data = &emissive_cube.material_data;

	directional_light.set_position(90.f, 160.f);
	directional_light.color         = {0.95f,0.85f,0.5f};
	directional_light.ambient_color = {0.95f,0.85f,0.5f};
	directional_light.ambient_strength = 0.1f;
	directional_light.brightness = 3.7f;

	// Setup Sun
	sun_material_.shader = forward_sun_;
	sun_material_.ubo = sun_material_ubo_;
	sun_material_.data = &sun_material_data_;
	sun_material_.data_size = sizeof(SunMaterialData);
	sun_material_data_.scale = 0.2f;

	camera_controller.set_position({0.f,1.f,3.f});
}

void Scene::shutdown()
{
	AssetManager::release(tg_);
	AssetManager::release(pbr_material_ubo_);
}

void Scene::update(erwin::GameClock& clock)
{
	float dt = clock.get_frame_duration();
	static float tt = 0.f;
	tt += dt;
	if(tt>=10.f)
		tt = 0.f;

	camera_controller.update(clock);

	// Update sun
	sun_material_data_.color = glm::vec4(directional_light.color, 1.f);
	sun_material_data_.brightness = directional_light.brightness;

	// Update cube
	float s = sin(2*M_PI*tt/10.f);
	float s2 = s*s;
	emissive_cube.material_data.emissive_scale = 1.f + 5.f * exp(-8.f*s2);
	emissive_cube.material_data.tint.r = 0.3f*exp(-12.f*s2);
}


} // namespace editor