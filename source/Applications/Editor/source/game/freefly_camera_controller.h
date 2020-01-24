#pragma once

#include "render/camera_3d.h"
#include "core/game_clock.h"
#include "event/window_events.h"

namespace game
{

class FreeflyController
{
public:
	FreeflyController(float aspect_ratio, float fovy, float znear, float zfar);
	~FreeflyController();

	void update(erwin::GameClock& clock);
	void toggle_control();

	inline const erwin::PerspectiveCamera3D& get_camera() const { return camera_; }
	inline erwin::PerspectiveCamera3D& get_camera()             { return camera_; }
	inline void set_position(const glm::vec3& value)            { camera_position_ = value; camera_.set_parameters(camera_position_, camera_yaw_, camera_pitch_); }

	bool on_window_resize_event(const erwin::WindowResizeEvent& event);
	bool on_window_moved_event(const erwin::WindowMovedEvent& event);
	bool on_mouse_scroll_event(const erwin::MouseScrollEvent& event);
	bool on_mouse_moved_event(const erwin::MouseMovedEvent& event);
	bool on_mouse_button_event(const erwin::MouseButtonEvent& event);

	inline float get_aspect_ratio() const { return aspect_ratio_; }
	inline float get_fovy() const         { return fovy_; }
	inline float get_znear() const        { return znear_; }
	inline float get_zfar() const         { return zfar_; }

private:
	erwin::PerspectiveCamera3D camera_;
	float aspect_ratio_;
	float fovy_;
	float znear_;
	float zfar_;
	float camera_translation_speed_;
	float camera_rotation_speed_;
	float camera_yaw_;
	float camera_pitch_;
	float win_width_;
	float win_height_;
	float win_x_;
	float win_y_;
	float prev_mouse_x_;
	float prev_mouse_y_;
	bool has_control_;
	glm::vec3 camera_position_;
};


} // namespace editor