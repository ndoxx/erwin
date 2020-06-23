#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

namespace erwin
{

struct Transform2D
{
	glm::vec3 position  = {0.f,0.f,0.f};
	float angle         = 0.f;
	float uniform_scale = 1.f;
};

struct Transform3D
{
	glm::vec3 position  = {0.f,0.f,0.f};
	glm::vec3 euler     = {0.f,0.f,0.f};
	glm::quat rotation  = glm::quat({0.f,0.f,0.f});
	float uniform_scale = 1.f;

	Transform3D() = default;

	// Euler angles are in the order: pitch, yaw, roll
	Transform3D(const glm::vec3& Position, const glm::vec3& Euler, float Uniform_scale):
	position(Position),
	euler(Euler),
	rotation(glm::radians(euler)),
	uniform_scale(Uniform_scale)
	{}

	inline void init(const glm::vec3& Position, const glm::vec3& Euler, float Uniform_scale)
	{
		position      = Position;
		euler         = Euler;
		rotation      = glm::quat(glm::radians(euler));
		uniform_scale = Uniform_scale;
	}

	inline void set_rotation(const glm::vec3& Euler)
	{
		euler = Euler;
		rotation = glm::quat(glm::radians(euler));
	}

	inline glm::mat4 get_model_matrix() const
	{
		return glm::translate(glm::mat4(1.f), position) 
			 * glm::toMat4(rotation)
		     * glm::scale(glm::mat4(1.f), glm::vec3(uniform_scale));
	}

	inline glm::mat4 get_unscaled_model_matrix() const
	{
		return glm::translate(glm::mat4(1.f), position) 
			 * glm::toMat4(rotation);
	}
};


} // namespace erwin