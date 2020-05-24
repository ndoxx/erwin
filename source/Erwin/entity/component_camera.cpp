#include "entity/component_camera.h"
#include "core/core.h"

#include "glm/gtc/matrix_access.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"

namespace erwin
{

void ComponentCamera3D::set_projection(const Frustum3D& _frustum)
{
    frustum = _frustum;
    projection_matrix =
        glm::frustum(frustum.left, frustum.right, frustum.bottom, frustum.top, frustum.near, frustum.far);
    projection_parameters = {1.0f / projection_matrix[0][0], 1.0f / projection_matrix[1][1],
                             projection_matrix[2][2] - 1.0f, projection_matrix[2][3]};
}

void ComponentCamera3D::update_transform(const glm::mat4& to_world_space)
{
    W_PROFILE_FUNCTION()

    // * Update frustum planes
    // Compute frustum corners in world space
    glm::vec3 rbn = to_world_space * glm::vec4(frustum.right, frustum.bottom, frustum.near, 1.f);
    glm::vec3 rbf = to_world_space * glm::vec4(frustum.right, frustum.bottom, frustum.far, 1.f);
    glm::vec3 lbf = to_world_space * glm::vec4(frustum.left, frustum.bottom, frustum.far, 1.f);
    glm::vec3 lbn = to_world_space * glm::vec4(frustum.left, frustum.bottom, frustum.near, 1.f);
    glm::vec3 rtn = to_world_space * glm::vec4(frustum.right, frustum.top, frustum.near, 1.f);
    // glm::vec3 rtf = to_world_space*glm::vec4(frustum.right, frustum.top,    frustum.far,  1.f);
    glm::vec3 ltf = to_world_space * glm::vec4(frustum.left, frustum.top, frustum.far, 1.f);
    glm::vec3 ltn = to_world_space * glm::vec4(frustum.left, frustum.top, frustum.near, 1.f);

    // Compute side plane coefficients from these points
    planes[0] = glm::normalize(glm::cross(lbf - lbn, ltn - lbn)); // left
    planes[1] = glm::normalize(glm::cross(rtn - rbn, rbf - rbn)); // right
    planes[2] = glm::normalize(glm::cross(rbn - lbn, lbf - lbn)); // bottom
    planes[3] = glm::normalize(glm::cross(ltf - ltn, rtn - ltn)); // top
    planes[4] = glm::normalize(glm::cross(ltn - lbn, rbn - lbn)); // near
    planes[5] = glm::normalize(glm::cross(rbf - lbf, ltf - lbf)); // far

    // * Update directions
    right   =  glm::vec3(glm::column(to_world_space, 0));
    up      =  glm::vec3(glm::column(to_world_space, 1));
    forward = -glm::vec3(glm::column(to_world_space, 2)); // Forward vector towards negative z values

    // * Update matrices
    view_matrix = glm::inverse(to_world_space);
    view_projection_matrix = projection_matrix * view_matrix;
}

} // namespace erwin