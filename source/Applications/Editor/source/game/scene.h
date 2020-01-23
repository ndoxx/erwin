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

	void add_entity(erwin::EntityID entity);

	erwin::EntityID directional_light;
	std::vector<erwin::EntityID> entities_;

	FreeflyController camera_controller;
	erwin::PostProcessingData post_processing;
};

} // namespace editor