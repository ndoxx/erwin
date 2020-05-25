#pragma once

#include "glm/glm.hpp"

namespace erwin
{

/*
	LEGACY
	TODO: Move this to a component when we need it
*/

struct Frustum2D
{
	float left;
	float right;
	float bottom;
	float top;
};

// Contains the side line coefficients of a given frustum in world space
struct FrustumSides
{
	FrustumSides() = default;
	FrustumSides(const Frustum2D& frustum, const glm::mat4& to_world_space);

	glm::vec3 side[4]; // left, right, bottom, top
};

class OrthographicCamera2D
{
public:
	explicit OrthographicCamera2D(const Frustum2D& frustum);

	inline const glm::vec2& get_position() const { return position_; }
	inline float get_angle() const { return angle_; }
	
	inline void set_position(const glm::vec2& value) { position_ = value; update_view_matrix(); }
	inline void set_angle(float value) { angle_ = value; update_view_matrix(); }
	inline void set_parameters(const glm::vec2& position, float angle) { position_ = position; angle_ = angle; update_view_matrix(); }

	inline const Frustum2D& get_frustum() const 			   { return frustum_; }
	inline const glm::mat4& get_transform() const              { return transform_; }
	inline const glm::mat4& get_view_matrix() const            { return view_matrix_; }
	inline const glm::mat4& get_projection_matrix() const      { return projection_matrix_; }
	inline const glm::mat4& get_view_projection_matrix() const { return view_projection_matrix_; }

	void set_projection(const Frustum2D& frustum);

	glm::vec2 get_up() const;
	glm::vec2 get_right() const;

	inline FrustumSides get_frustum_sides() const { return FrustumSides(frustum_, transform_); }

private:
	void update_view_matrix();

private:
	Frustum2D frustum_;
	float angle_;
	glm::vec2 position_;
	glm::mat4 transform_;
	glm::mat4 view_matrix_;
	glm::mat4 projection_matrix_;
	glm::mat4 view_projection_matrix_;
};


} // namespace erwin