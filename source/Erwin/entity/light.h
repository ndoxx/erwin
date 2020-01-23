#pragma once

#include "entity/component.h"
#include "glm/glm.hpp"

namespace erwin
{

class ComponentDirectionalLight: public Component
{
public:
	COMPONENT_DECLARATION(ComponentDirectionalLight);

	virtual bool init(void* description) override final;
	virtual void inspector_GUI() override final;

	// Set position from orbital parameters
	inline void set_position(float inclination_deg, float arg_periapsis_deg)
	{
		float inclination   = glm::radians(inclination_deg);
		float arg_periapsis = glm::radians(arg_periapsis_deg);
		position = {cos(inclination),sin(inclination)*sin(arg_periapsis),sin(inclination)*cos(arg_periapsis)};
	}

	glm::vec3 position;
	glm::vec3 color;
	glm::vec3 ambient_color;
	float ambient_strength;
	float brightness = 1.f;
};

} // namespace erwin