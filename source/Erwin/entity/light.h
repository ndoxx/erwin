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
		float inclination   = glm::radians(inclination_deg);
		float arg_periapsis = glm::radians(arg_periapsis_deg);
		position = {std::cos(inclination),std::sin(inclination)*std::sin(arg_periapsis),std::sin(inclination)*std::cos(arg_periapsis)};
	}

	glm::vec3 position      = {0.f,0.f,0.f};
	glm::vec3 color         = {1.f,1.f,1.f};
	glm::vec3 ambient_color = {1.f,1.f,1.f};
	float ambient_strength  = 0.05f;
	float brightness        = 1.f;
};

template <> [[maybe_unused]] void inspector_GUI<ComponentDirectionalLight>(ComponentDirectionalLight*);

} // namespace erwin