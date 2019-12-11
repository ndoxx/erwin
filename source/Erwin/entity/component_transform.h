#pragma once

#include "entity/component.h"
#include "glm/glm.hpp"

namespace erwin
{

COUNTER_INC(ComponentAutoCounter);
class ComponentTransform2D: public Component
{
public:
	static constexpr ComponentID ID = COUNTER_READ(ComponentAutoCounter) - 1;

	glm::vec2 position;
	float angle;
	float uniform_scale;

	ComponentTransform2D(): position(0.f), angle(0.f), uniform_scale(1.f) {}
	virtual bool init(void* description) override final;

	POOL_DECLARATION(32)
};


} // namespace erwin