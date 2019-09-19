#pragma once

#include "glm/glm.hpp"

namespace erwin
{

struct Frustum2D
{
	float left;
	float right;
	float top;
	float bottom;
};

class OrthographicCamera2D
{
public:
	OrthographicCamera2D(const Frustum2D& frustum);
	~OrthographicCamera2D();

	inline const glm::vec2& get_position() const	 { return position_; }
	inline void set_position(const glm::vec2& value) { position_ = value; update_view_matrix(); }

	inline float get_angle() const	   { return angle_; }
	inline void set_angle(float value) { angle_ = value; update_view_matrix(); }

	inline const Frustum2D& get_frustum() const 			   { return frustum_; }
	inline const glm::mat4& get_view_matrix() const            { return view_matrix_; }
	inline const glm::mat4& get_projection_matrix() const      { return projection_matrix_; }
	inline const glm::mat4& get_view_projection_matrix() const { return view_projection_matrix_; }

	void set_projection(const Frustum2D& frustum);

	glm::vec2 get_up() const;
	glm::vec2 get_right() const;

private:
	void update_view_matrix();

private:
	Frustum2D frustum_;
	float angle_;
	glm::vec2 position_;
	glm::mat4 view_matrix_;
	glm::mat4 projection_matrix_;
	glm::mat4 view_projection_matrix_;
};


} // namespace erwin