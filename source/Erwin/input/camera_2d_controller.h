#pragma once

#include "render/camera_2d.h"
#include "core/game_clock.h"
#include "event/window_events.h"

namespace erwin
{

class OrthographicCamera2DController
{
public:
	explicit OrthographicCamera2DController(float aspect_ratio, float zoom_level=1.f);

	void update(const GameClock& clock);

	inline const OrthographicCamera2D& get_camera() const { return camera_; }
	inline OrthographicCamera2D& get_camera()             { return camera_; }

	inline float get_zoom_level() const   { return zoom_level_; }
	inline float get_aspect_ratio() const { return aspect_ratio_; }

	bool on_window_resize_event(const WindowResizeEvent& event);
	bool on_mouse_scroll_event(const MouseScrollEvent& event);

private:
	OrthographicCamera2D camera_;
	float aspect_ratio_;
	float zoom_level_;
	float camera_translation_speed_;
	float camera_rotation_speed_;
	float camera_angle_;
	glm::vec2 camera_position_;
};


} // namespace erwin