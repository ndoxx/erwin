#pragma once

#include "entity/component.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

namespace erwin
{

class ComponentTransform2D: public Component
{
public:
	ID_DECLARATION(ComponentTransform2D);
	POOL_DECLARATION(ComponentTransform2D);

	glm::vec2 position;
	float angle;
	float uniform_scale;

	ComponentTransform2D(): position(0.f), angle(0.f), uniform_scale(1.f) {}
	virtual bool init(void* description) override final;
};

class ComponentTransform3D: public Component
{
public:
	ID_DECLARATION(ComponentTransform3D);
	POOL_DECLARATION(ComponentTransform3D);

	glm::vec3 position;
	glm::quat rotation;
	float uniform_scale;

	ComponentTransform3D(): position(0.f), uniform_scale(1.f) {}
	virtual bool init(void* description) override final;
};

} // namespace erwin