#pragma once

#include "glm/glm.hpp"

namespace erwin
{

struct DirectionalLight
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec3 ambient_color;
	float ambient_strength;
	float brightness = 1.f;

	// Set position from orbital parameters
	inline void set_position(float inclination_deg, float arg_periapsis_deg)
	{
		float inclination   = glm::radians(inclination_deg);
		float arg_periapsis = glm::radians(arg_periapsis_deg);
		position = {cos(inclination),sin(inclination)*sin(arg_periapsis),sin(inclination)*cos(arg_periapsis)};
	}
};

} // namespace erwin