#pragma once

#include "erwin.h"
#include "game/freefly_camera_controller.h"

namespace game
{

struct SunMaterialData
{
	glm::vec4 color;
	float scale;
	float brightness;
};

class Scene
{
public:
	Scene();

	void init(erwin::EntityManager& emgr);
	void shutdown();
	void update(erwin::GameClock& clock);

	erwin::EntityID cube_ent;

	FreeflyController camera_controller;
	erwin::PostProcessingData post_processing;
	erwin::DirectionalLight directional_light;

	erwin::Material sun_material_;
	SunMaterialData sun_material_data_;

private:
	erwin::TextureGroupHandle tg_;
	erwin::UniformBufferHandle sun_material_ubo_;
	erwin::UniformBufferHandle pbr_material_ubo_;
	erwin::ShaderHandle forward_sun_;
	erwin::ShaderHandle deferred_pbr_;
};

} // namespace editor