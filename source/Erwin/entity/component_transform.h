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
	COMPONENT_DECLARATION(ComponentTransform2D);

	glm::vec3 position;
	float angle;
	float uniform_scale;

	ComponentTransform2D(): position(0.f), angle(0.f), uniform_scale(1.f) {}
	ComponentTransform2D(const glm::vec3& position, float angle, float uniform_scale): position(position), angle(angle), uniform_scale(uniform_scale) {}
	virtual bool init(void* description) override final;
	virtual void inspector_GUI() override final;
};

class ComponentTransform3D: public Component
{
public:
	COMPONENT_DECLARATION(ComponentTransform3D);

	glm::vec3 position;
	glm::vec3 euler;
	glm::quat rotation;
	float uniform_scale;

	ComponentTransform3D(): position(0.f), rotation({0.f,0.f,0.f}), uniform_scale(1.f) {}
	// Euler angles are in the order: pitch, yaw, roll
	ComponentTransform3D(const glm::vec3& position, const glm::vec3& euler_angles, float uniform_scale):
	position(position),
	euler(euler_angles),
	rotation(glm::radians(euler)),
	uniform_scale(uniform_scale)
	{}

	inline void set_rotation(const glm::vec3& euler_angles)
	{
		euler = euler_angles;
		rotation = glm::quat(glm::radians(euler));
	}

	inline glm::mat4 get_model_matrix() const
	{
		return glm::translate(glm::mat4(1.f), position) 
			 * glm::toMat4(rotation)
		     * glm::scale(glm::mat4(1.f), glm::vec3(uniform_scale));
	}

	virtual bool init(void* description) override final;
	virtual void inspector_GUI() override final;
};

} // namespace erwin