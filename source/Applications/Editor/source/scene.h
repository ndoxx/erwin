#pragma once

#include "erwin.h"
#include "freefly_camera_controller.h"

namespace editor
{

struct PBRMaterialData
{
	inline void enable_emissivity() { flags |= (1<<0); }

	glm::vec4 tint;
	int flags;
	float emissive_scale;
};

struct SunMaterialData
{
	glm::vec4 color;
	float scale;
	float brightness;
};

struct Cube
{
	erwin::ComponentTransform3D transform;
	erwin::Material material;
	PBRMaterialData material_data;
};

class Scene
{
public:
	Scene();

	void init();
	void shutdown();
	void update(erwin::GameClock& clock);

	FreeflyController camera_controller;
	erwin::DirectionalLight directional_light;
	Cube emissive_cube;
	erwin::Material sun_material_;
	erwin::PostProcessingData post_processing;

private:
	erwin::TextureGroupHandle tg_;
	erwin::UniformBufferHandle sun_material_ubo_;
	erwin::UniformBufferHandle pbr_material_ubo_;
	erwin::ShaderHandle forward_sun_;
	erwin::ShaderHandle deferred_pbr_;
	SunMaterialData sun_material_data_;
};

} // namespace editor