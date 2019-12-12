#pragma once

#include "entity/component.h"
#include "glm/glm.hpp"

namespace erwin
{

class ComponentTransform2D: public Component
{
public:
	ID_DECLARATION(ComponentTransform2D);

	glm::vec2 position;
	float angle;
	float uniform_scale;

	ComponentTransform2D(): position(0.f), angle(0.f), uniform_scale(1.f) {}
	virtual bool init(void* description) override final;

	POOL_DECLARATION(32)
};


} // namespace erwin