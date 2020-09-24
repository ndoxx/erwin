#pragma once

#include "math/transform.h"

namespace erwin
{

struct ComponentTransform2D
{
    Transform2D local;
    Transform2D global;
};

struct ComponentTransform3D
{
	ComponentTransform3D() = default;

    ComponentTransform3D(const glm::vec3& position, const glm::vec3& euler, float uniform_scale)
        : local(position, euler, uniform_scale)
    {}

    ComponentTransform3D(const glm::vec3& position, const glm::vec3& euler)
        : local(position, euler, 1.f)
    {}

    explicit ComponentTransform3D(const glm::vec3& position)
        : local(position, {0.f,0.f,0.f}, 1.f)
    {}

    Transform3D local{};
    Transform3D global{};
};

} // namespace erwin