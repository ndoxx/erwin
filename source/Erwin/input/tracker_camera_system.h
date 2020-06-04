#pragma once

#include "entity/component_camera.h"
#include "entity/component_transform.h"
#include "core/game_clock.h"
#include "event/window_events.h"
#include "entity/reflection.h"

namespace erwin
{

class TrackerCameraSystem
{
public:
	struct FrustumParameters
	{
		float aspect_ratio = 1280.f / 1024.f;
		float fovy = 60.f;
		float znear = 0.1f;
		float zfar = 100.f;
	};

	TrackerCameraSystem();
	void init(ComponentTransform3D& transform);
	void set_frustum_parameters(const FrustumParameters& params);
	void set_position(float radius, float azimuth, float colatitude);
	void update(const erwin::GameClock& clock, ComponentCamera3D& camera, ComponentTransform3D& transform);

	bool on_window_resize_event(const erwin::WindowResizeEvent& event);
	bool on_window_moved_event(const erwin::WindowMovedEvent& event);
	bool on_mouse_scroll_event(const erwin::MouseScrollEvent& event);
	bool on_mouse_moved_event(const erwin::MouseMovedEvent& event);
	bool on_mouse_button_event(const erwin::MouseButtonEvent& event);
	bool on_keyboard_event(const erwin::KeyboardEvent& event);

	inline void set_lookat_target(const glm::vec3& target) { lookat_target_ = target; }
	inline float get_aspect_ratio() const { return frustum_parameters_.aspect_ratio; }
	inline float get_fovy() const         { return frustum_parameters_.fovy; }
	inline float get_znear() const        { return frustum_parameters_.znear; }
	inline float get_zfar() const         { return frustum_parameters_.zfar; }

private:
	void update_frustum();

private:
	FrustumParameters frustum_parameters_;
	ComponentCamera3D::Frustum3D frustum_;

	float rotation_speed_;
	float azimuth_;
	float colatitude_;
	float radius_;
	float win_width_;
	float win_height_;
	float win_x_;
	float win_y_;
	float prev_mouse_x_;
	float prev_mouse_y_;
	bool inputs_enabled_;
	bool dirty_frustum_;
	glm::vec3 position_;
	glm::vec3 lookat_target_;
};


} // namespace erwin