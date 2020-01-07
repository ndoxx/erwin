#pragma once

#include <tuple>
#include "glm/glm.hpp"

namespace erwin
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
	FrustumPlanes();
	FrustumPlanes(const Frustum3D& frustum, const glm::mat4& to_world_space);

	glm::vec3 plane[6]; // left, right, bottom, top, near, far
};

class PerspectiveCamera3D
{
public:
	PerspectiveCamera3D(const Frustum3D& frustum);
	~PerspectiveCamera3D();

	inline const glm::vec3& get_position() const { return position_; }
	inline float get_yaw() const   { return yaw_; }
	inline float get_pitch() const { return pitch_; }
	inline float get_roll() const  { return roll_; }
	inline glm::vec3 get_angles() const { return {yaw_, pitch_, roll_}; }

	inline void set_position(const glm::vec3& value) { position_ = value; update_view_matrix(); }
	inline void set_yaw(float value)   { yaw_ = value; update_view_matrix(); }
	inline void set_pitch(float value) { pitch_ = value; update_view_matrix(); }
	inline void set_roll(float value)  { roll_ = value; update_view_matrix(); }
	inline void set_angles(float yaw, float pitch, float roll) { yaw_ = yaw; pitch_ = pitch; roll_ = roll; update_view_matrix(); }
	inline void set_parameters(const glm::vec3& position, float yaw, float pitch, float roll = 0.f)
	{
		position_ = position;
		yaw_ = yaw;
		pitch_ = pitch;
		roll_ = roll;
		update_view_matrix();
	}

	inline const Frustum3D& get_frustum() const 			   { return frustum_; }
	inline const glm::mat4& get_transform() const              { return transform_; }
	inline const glm::mat4& get_view_matrix() const            { return view_matrix_; }
	inline const glm::mat4& get_projection_matrix() const      { return projection_matrix_; }
	inline const glm::mat4& get_view_projection_matrix() const { return view_projection_matrix_; }

	void set_projection(const Frustum3D& frustum);

	glm::vec3 get_right() const;
	glm::vec3 get_up() const;
	glm::vec3 get_forward() const;
	glm::vec4 get_projection_parameters() const;

	inline FrustumPlanes get_frustum_planes() const { return FrustumPlanes(frustum_, transform_); }

private:
	void update_view_matrix();

private:
	Frustum3D frustum_;
	float yaw_;
	float pitch_;
	float roll_;
	glm::vec3 position_;
	glm::mat4 transform_;
	glm::mat4 view_matrix_;
	glm::mat4 projection_matrix_;
	glm::mat4 view_projection_matrix_;
};


} // namespace erwin