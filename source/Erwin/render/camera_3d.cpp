#include "render/camera_3d.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_access.hpp"
#include "glm/gtx/euler_angles.hpp"

namespace erwin
{

FrustumPlanes::FrustumPlanes() {  }

FrustumPlanes::FrustumPlanes(const Frustum3D& frustum, const glm::mat4& to_world_space)
{
	// Compute frustum corners in world space
    glm::vec3 rbn = to_world_space*glm::vec4(frustum.right, frustum.bottom, frustum.near, 1.f);
    glm::vec3 rbf = to_world_space*glm::vec4(frustum.right, frustum.bottom, frustum.far,  1.f);
    glm::vec3 lbf = to_world_space*glm::vec4(frustum.left,  frustum.bottom, frustum.far,  1.f);
    glm::vec3 lbn = to_world_space*glm::vec4(frustum.left,  frustum.bottom, frustum.near, 1.f);
    glm::vec3 rtn = to_world_space*glm::vec4(frustum.right, frustum.top,    frustum.near, 1.f);
    // glm::vec3 rtf = to_world_space*glm::vec4(frustum.right, frustum.top,    frustum.far,  1.f);
    glm::vec3 ltf = to_world_space*glm::vec4(frustum.left,  frustum.top,    frustum.far,  1.f);
    glm::vec3 ltn = to_world_space*glm::vec4(frustum.left,  frustum.top,    frustum.near, 1.f);

	// Compute side plane coefficients from these points
	plane[0] = glm::normalize(glm::cross(lbf-lbn, ltn-lbn)); // left
	plane[1] = glm::normalize(glm::cross(rtn-rbn, rbf-rbn)); // right
	plane[2] = glm::normalize(glm::cross(rbn-lbn, lbf-lbn)); // bottom
	plane[3] = glm::normalize(glm::cross(ltf-ltn, rtn-ltn)); // top
	plane[4] = glm::normalize(glm::cross(ltn-lbn, rbn-lbn)); // near
	plane[5] = glm::normalize(glm::cross(rbf-lbf, ltf-lbf)); // far
}

PerspectiveCamera3D::PerspectiveCamera3D(const Frustum3D& frustum):
pitch_(0.f),
yaw_(0.f),
position_(0.f)
{
	set_projection(frustum);
}

PerspectiveCamera3D::~PerspectiveCamera3D()
{
	
}

void PerspectiveCamera3D::set_projection(const Frustum3D& frustum)
{
	frustum_ = frustum;
	projection_matrix_ = glm::frustum(frustum.left, frustum.right, frustum.bottom, frustum.top, frustum.near, frustum.far);
	update_view_matrix();
}

void PerspectiveCamera3D::update_view_matrix()
{
	transform_ = glm::translate(glm::mat4(1.f), position_)
			   * glm::eulerAngleYXZ(yaw_, pitch_, 0.f);
	view_matrix_ = glm::inverse(transform_);
	view_projection_matrix_ = projection_matrix_*view_matrix_;
}

glm::vec3 PerspectiveCamera3D::get_right() const
{
	return glm::vec3(glm::column(view_matrix_, 0));
}

glm::vec3 PerspectiveCamera3D::get_up() const
{
	return glm::vec3(glm::column(view_matrix_, 1));
}

glm::vec3 PerspectiveCamera3D::get_forward() const
{
	return glm::vec3(glm::column(view_matrix_, 2));
}


} // namespace erwin