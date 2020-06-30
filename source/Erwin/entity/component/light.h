#pragma once

#include "entity/reflection.h"
#include "glm/glm.hpp"

namespace erwin
{

struct ComponentDirectionalLight
{
	// Set position from orbital parameters
	inline void set_position(float inclination_deg, float arg_periapsis_deg)
	{
		inclination = inclination_deg;
		arg_periapsis = arg_periapsis_deg;
		float incl = glm::radians(inclination_deg);
		float ap = glm::radians(arg_periapsis_deg);
		position = {std::cos(incl),std::sin(incl)*std::sin(ap),std::sin(incl)*std::cos(ap)};
	}

	glm::vec3 position      = {0.f,0.f,0.f};
	glm::vec3 color         = {1.f,1.f,1.f};
	glm::vec3 ambient_color = {1.f,1.f,1.f};
	float ambient_strength  = 0.05f;
	float brightness        = 1.f;
	float inclination       = 0.f;
	float arg_periapsis     = 0.f;
};

} // namespace erwin