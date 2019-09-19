#include "render/camera_2d.h"

#include "glm/gtc/matrix_transform.hpp"

namespace erwin
{

OrthographicCamera2D::OrthographicCamera2D(const Frustum2D& frustum):
angle_(0.f),
position_(0.f)
{
	set_projection(frustum);
}

OrthographicCamera2D::~OrthographicCamera2D()
{
	
}

void OrthographicCamera2D::set_projection(const Frustum2D& frustum)
{
	frustum_ = frustum;
	projection_matrix_ = glm::ortho(frustum.left, frustum.right, frustum.bottom, frustum.top, -1.f, 1.f);
	update_view_matrix();
}

void OrthographicCamera2D::update_view_matrix()
{
	glm::mat4 transform = glm::translate(glm::mat4(1.f), glm::vec3(position_,0.f))
						* glm::rotate(glm::mat4(1.f), glm::radians(angle_), glm::vec3(0.f,0.f,1.f));
	view_matrix_ = glm::inverse(transform);
	view_projection_matrix_ = projection_matrix_*view_matrix_;
}

glm::vec2 OrthographicCamera2D::get_up() const
{
	return glm::vec2(-sin(glm::radians(angle_)), cos(glm::radians(angle_)));
}

glm::vec2 OrthographicCamera2D::get_right() const
{
	return glm::vec2(cos(glm::radians(angle_)), sin(glm::radians(angle_)));
}

} // namespace erwin