#include "input/rotate_around_camera_controller.h"
#include "input/input.h"
#include "debug/logger.h"

#include "glm/gtx/string_cast.hpp"

namespace erwin
{
using namespace keymap;

// Helper function to get "top" dimension from FOV and z-near
inline static float fovy_znear_to_top(float fovy, float znear)
{
	return znear * std::tan(0.5f * fovy * (float(M_PI) / 180.f));
}

RotateAroundController::RotateAroundController(float aspect_ratio, float fovy, float znear, float zfar):
win_width_(0.f),
win_height_(0.f)
{
	init(aspect_ratio, fovy, znear, zfar);
}

void RotateAroundController::init(float aspect_ratio, float fovy, float znear, float zfar)
{
	camera_.init({-aspect_ratio*fovy_znear_to_top(fovy,znear), 
				   aspect_ratio*fovy_znear_to_top(fovy,znear),
	              -fovy_znear_to_top(fovy,znear), 
	               fovy_znear_to_top(fovy,znear),
	               znear, 
	               zfar});

	aspect_ratio_ = aspect_ratio;
	fovy_         = fovy;
	znear_        = znear;
	zfar_         = zfar;
	camera_rotation_speed_ = 2.5f * float(M_PI) / 180.f;
	camera_azimuth_        = 0.f;
	camera_colatitude_     = 90.f;
	camera_radius_         = 5.f;
	camera_target_         = {0.f,0.f,0.f};

	set_position(camera_radius_, camera_azimuth_, camera_colatitude_);

	win_x_ = 0.f;
	win_y_ = 0.f;
	inputs_enabled_ = false;
	prev_mouse_x_ = 0.f;
	prev_mouse_y_ = 0.f;
}

void RotateAroundController::update(const GameClock&)
{
	camera_.look_at(camera_position_, camera_target_);
}

void RotateAroundController::enable_inputs(bool)
{
	inputs_enabled_ = !inputs_enabled_;
	Input::show_cursor(!inputs_enabled_);

	// Save mouse position if control was acquired, restore it if control was released
	if(inputs_enabled_)
	{
		std::tie(prev_mouse_x_, prev_mouse_y_) = Input::get_mouse_position();
		Input::set_mouse_position(win_x_+0.5f*win_width_, win_y_+0.5f*win_height_);
	}
	else
		Input::set_mouse_position(prev_mouse_x_, prev_mouse_y_);
}

bool RotateAroundController::on_window_resize_event(const WindowResizeEvent& event)
{
	win_width_ = float(event.width);
	win_height_ = float(event.height);
	aspect_ratio_ = float(event.width)/float(event.height);
	float top = fovy_znear_to_top(fovy_, znear_);
	camera_.set_projection({-aspect_ratio_*top, aspect_ratio_*top, -top, top, znear_, zfar_});
	return false;
}

bool RotateAroundController::on_window_moved_event(const erwin::WindowMovedEvent& event)
{
	win_x_ = float(event.x);
	win_y_ = float(event.y);
	return false;
}

bool RotateAroundController::on_mouse_scroll_event(const MouseScrollEvent& event)
{
	if(!inputs_enabled_)
		return false;

	// Update radius
	float factor = (event.y_offset<0) ? 1.05f : 0.95f;
	set_position(camera_radius_*factor, camera_azimuth_, camera_colatitude_);

	return true;
}

bool RotateAroundController::on_mouse_button_event(const MouseButtonEvent&)
{
	return false;
}

void RotateAroundController::set_position(float radius, float azimuth, float colatitude)
{
	camera_radius_ = radius;
	camera_azimuth_ = azimuth;
	camera_colatitude_ = colatitude;

	camera_azimuth_ = (camera_azimuth_>360.f) ? camera_azimuth_-360.f : camera_azimuth_;
	camera_azimuth_ = (camera_azimuth_<0.f)   ? 360.f-camera_azimuth_ : camera_azimuth_;
	camera_colatitude_ = (camera_colatitude_> 179.f) ?  179.f : camera_colatitude_;
	camera_colatitude_ = (camera_colatitude_<1.f) ? 1.f : camera_colatitude_;

	camera_position_ = {camera_radius_*std::sin(glm::radians(camera_colatitude_))*std::cos(glm::radians(camera_azimuth_)),
						camera_radius_*std::cos(glm::radians(camera_colatitude_)),
						camera_radius_*std::sin(glm::radians(camera_colatitude_))*std::sin(glm::radians(camera_azimuth_))};
}

bool RotateAroundController::on_mouse_moved_event(const MouseMovedEvent& event)
{
	if(!inputs_enabled_)
		return false;

	float dphi   = camera_rotation_speed_ * float(event.x - (win_x_+0.5f*win_width_));
	float dtheta = camera_rotation_speed_ * float(event.y - (win_y_+0.5f*win_height_));

	set_position(camera_radius_, camera_azimuth_ - dphi, camera_colatitude_ + dtheta);

	// Set cursor back to the center of the screen
	Input::set_mouse_position(win_x_+0.5f*win_width_, win_y_+0.5f*win_height_);
	// Input::center_mouse_position();

	return true;
}

bool RotateAroundController::on_keyboard_event(const erwin::KeyboardEvent& event)
{
	if(event.pressed && !event.repeat && event.key == Input::get_action_key(ACTION_FREEFLY_TOGGLE))
	{
		toggle_inputs();
		return true;
	}

	return false;
}


} // namespace erwin