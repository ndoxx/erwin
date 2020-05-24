#pragma once

#include <tuple>
#include "glm/glm.hpp"

namespace erwin
{

struct ComponentCamera3D
{
    struct Frustum3D
    {
        float left;
        float right;
        float bottom;
        float top;
        float near;
        float far;
    };

    struct FrustumPlanes
    {
        inline const glm::vec3& operator[] (size_t index) const { return plane[index]; }
        inline glm::vec3& operator[] (size_t index)             { return plane[index]; }
        glm::vec3 plane[6]; // left, right, bottom, top, near, far
    };

    void set_projection(const Frustum3D&);
    void update_transform(const glm::mat4& to_world_space);

    Frustum3D frustum;
    FrustumPlanes planes;
    glm::vec3 right;
    glm::vec3 up;
    glm::vec3 forward;
    glm::vec4 projection_parameters;
    glm::mat4 view_matrix;
    glm::mat4 projection_matrix;
    glm::mat4 view_projection_matrix;
};

} // namespace erwin
