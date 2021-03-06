#include "render/camera_2d.h"
#include "core/core.h"

#include "glm/gtc/matrix_transform.hpp"

namespace erwin
{

FrustumSides::FrustumSides(const Frustum2D& frustum, const glm::mat4& to_world_space)
{
	W_PROFILE_FUNCTION()

	// Compute frustum corners in world space
	glm::vec4 bl = to_world_space*glm::vec4(frustum.left, frustum.bottom, 0.f, 1.f);
	glm::vec4 br = to_world_space*glm::vec4(frustum.right, frustum.bottom, 0.f, 1.f);
	glm::vec4 tl = to_world_space*glm::vec4(frustum.left, frustum.top, 0.f, 1.f);
	glm::vec4 tr = to_world_space*glm::vec4(frustum.right, frustum.top, 0.f, 1.f);

	// Compute side line coefficients from these points
	side[0] = glm::vec3(tl.y-bl.y, bl.x-tl.x, tl.x*bl.y-bl.x*tl.y); // left
	side[1] = glm::vec3(br.y-tr.y, tr.x-br.x, br.x*tr.y-tr.x*br.y); // right
	side[2] = glm::vec3(bl.y-br.y, br.x-bl.x, bl.x*br.y-br.x*bl.y); // bottom
	side[3] = glm::vec3(tr.y-tl.y, tl.x-tr.x, tr.x*tl.y-tl.x*tr.y); // top
}


OrthographicCamera2D::OrthographicCamera2D(const Frustum2D& frustum):
angle_(0.f),
position_(0.f)
{
	set_projection(frustum);
}

void OrthographicCamera2D::set_projection(const Frustum2D& frustum)
{
	frustum_ = frustum;
	projection_matrix_ = glm::ortho(frustum.left, frustum.right, frustum.bottom, frustum.top, -1.f, 1.f);
	update_view_matrix();
}

void OrthographicCamera2D::update_view_matrix()
{
	W_PROFILE_FUNCTION()

	transform_ = glm::translate(glm::mat4(1.f), glm::vec3(position_,0.f))
			   * glm::rotate(glm::mat4(1.f), glm::radians(angle_), glm::vec3(0.f,0.f,1.f));
	view_matrix_ = glm::inverse(transform_);
	view_projection_matrix_ = projection_matrix_*view_matrix_;
}

glm::vec2 OrthographicCamera2D::get_up() const
{
	return glm::vec2(-std::sin(glm::radians(angle_)), std::cos(glm::radians(angle_)));
}

glm::vec2 OrthographicCamera2D::get_right() const
{
	return glm::vec2(std::cos(glm::radians(angle_)), std::sin(glm::radians(angle_)));
}

} // namespace erwin