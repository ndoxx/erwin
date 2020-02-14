#pragma once

#include "render/camera_3d.h"
#include "core/game_clock.h"
#include "event/window_events.h"

namespace erwin
{

class FreeflyController
{
public:
	FreeflyController() = default;
	FreeflyController(float aspect_ratio, float fovy, float znear, float zfar);
	~FreeflyController();

	void init(float aspect_ratio, float fovy, float znear, float zfar);

	void update(erwin::GameClock& clock);
	void enable_inputs(bool value);
	inline bool is_enabled() const { return inputs_enabled_; }
	inline void toggle_inputs()    { enable_inputs(!inputs_enabled_); }

	inline const erwin::PerspectiveCamera3D& get_camera() const { return camera_; }
	inline erwin::PerspectiveCamera3D& get_camera()             { return camera_; }
	inline void set_position(const glm::vec3& value)            { camera_position_ = value; camera_.set_parameters(camera_position_, camera_yaw_, camera_pitch_); }

	bool on_window_resize_event(const erwin::WindowResizeEvent& event);
	bool on_window_moved_event(const erwin::WindowMovedEvent& event);
	bool on_mouse_scroll_event(const erwin::MouseScrollEvent& event);
	bool on_mouse_moved_event(const erwin::MouseMovedEvent& event);
	bool on_mouse_button_event(const erwin::MouseButtonEvent& event);
	bool on_keyboard_event(const erwin::KeyboardEvent& event);

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
	bool inputs_enabled_;
	glm::vec3 camera_position_;
};


} // namespace erwin