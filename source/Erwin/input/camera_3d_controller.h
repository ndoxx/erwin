#pragma once

#include "render/camera_3d.h"
#include "core/game_clock.h"
#include "event/window_events.h"

namespace erwin
{

class PerspectiveFreeflyController
{
public:
	PerspectiveFreeflyController(float aspect_ratio, float fovy, float znear, float zfar);

	void update(GameClock& clock);

	inline const PerspectiveCamera3D& get_camera() const { return camera_; }
	inline PerspectiveCamera3D& get_camera()             { return camera_; }
	inline void set_position(const glm::vec3& value)     { camera_position_ = value; }

	bool on_window_resize_event(const WindowResizeEvent& event);
	bool on_mouse_scroll_event(const MouseScrollEvent& event);
	bool on_mouse_moved_event(const MouseMovedEvent& event);
	bool on_mouse_button_event(const MouseButtonEvent& event);

private:
	PerspectiveCamera3D camera_;
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
	glm::vec3 camera_position_;
};


} // namespace erwin