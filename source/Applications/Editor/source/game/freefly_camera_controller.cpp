#include "game/freefly_camera_controller.h"
#include "input/input.h"
#include "debug/logger.h"

#include "glm/gtx/string_cast.hpp"

using namespace erwin;
using namespace keymap;

namespace game
{

// Helper function to get "top" dimension from FOV and z-near
inline float fovy_znear_to_top(float fovy, float znear)
{
	return znear * tan(0.5f * fovy * (M_PI / 180.f));
}

FreeflyController::FreeflyController(float aspect_ratio, float fovy, float znear, float zfar):
camera_({-aspect_ratio*fovy_znear_to_top(fovy,znear), aspect_ratio*fovy_znear_to_top(fovy,znear),
         -fovy_znear_to_top(fovy,znear), fovy_znear_to_top(fovy,znear),
         znear, zfar}),
aspect_ratio_(aspect_ratio),
fovy_(fovy),
znear_(znear),
zfar_(zfar),
camera_translation_speed_(2.f),
camera_rotation_speed_(2.5f * M_PI / 180.f),
camera_yaw_(camera_.get_yaw()),
camera_pitch_(camera_.get_pitch()),
camera_position_(camera_.get_position())
{
	has_control_ = false;
	prev_mouse_x_ = 0.f;
	prev_mouse_y_ = 0.f;
}

FreeflyController::~FreeflyController()
{
	
}

void FreeflyController::update(GameClock& clock)
{
	if(!has_control_)
		return;

	// Translational magnitude
	float dt = clock.get_frame_duration();
	float speed_modifier = Input::is_key_pressed(WKEY::LEFT_SHIFT) ? 5.f : 1.f;
	float translation = dt * speed_modifier * camera_translation_speed_;

	// Front direction is the normalized projection of the camera forward axis on the horizontal plane
	glm::vec3 front = camera_.get_forward();
	front.y = 0.f;
	front = glm::normalize(front);

	// Handle keyboard inputs
	if(Input::is_key_pressed(WKEY::W)) // FORWARD
		camera_position_ += translation*front;
	if(Input::is_key_pressed(WKEY::S)) // BACKWARD
		camera_position_ -= translation*front;
	if(Input::is_key_pressed(WKEY::A)) // LEFT
		camera_position_ -= translation*camera_.get_right();
	if(Input::is_key_pressed(WKEY::D)) // RIGHT
		camera_position_ += translation*camera_.get_right();
	if(Input::is_key_pressed(WKEY::SPACE)) // UP
		camera_position_ += translation*glm::vec3(0.f,1.f,0.f);
	if(Input::is_key_pressed(WKEY::LEFT_CONTROL)) // DOWN
		camera_position_ -= translation*glm::vec3(0.f,1.f,0.f);

	// Update camera parameters
	camera_.set_parameters(camera_position_, camera_yaw_, camera_pitch_);
}

void FreeflyController::toggle_control()
{
	has_control_ = !has_control_;
	Input::show_cursor(!has_control_);

	// Save mouse position if control was acquired, restore it if control was released
	if(has_control_)
	{
		std::tie(prev_mouse_x_, prev_mouse_y_) = Input::get_mouse_position();
		Input::set_mouse_position(win_x_+0.5f*win_width_, win_y_+0.5f*win_height_);
	}
	else
		Input::set_mouse_position(prev_mouse_x_, prev_mouse_y_);
}

bool FreeflyController::on_window_resize_event(const WindowResizeEvent& event)
{
	win_width_ = event.width;
	win_height_ = event.height;
	aspect_ratio_ = event.width/float(event.height);
	float top = fovy_znear_to_top(fovy_, znear_);
	camera_.set_projection({-aspect_ratio_*top, aspect_ratio_*top, -top, top, znear_, zfar_});
	return false;
}

bool FreeflyController::on_window_moved_event(const erwin::WindowMovedEvent& event)
{
	win_x_ = event.x;
	win_y_ = event.y;
	return false;
}

bool FreeflyController::on_mouse_scroll_event(const MouseScrollEvent& event)
{
	if(!has_control_)
		return false;

	// Update and constrain FOV
	fovy_ *= (event.y_offset<0) ? 1.05f : 0.95f;
	fovy_  = (fovy_>120.f) ? 120.f : fovy_;
	fovy_  = (fovy_<20.f)  ? 20.f  : fovy_;

	float top = fovy_znear_to_top(fovy_, znear_);
	camera_.set_projection({-aspect_ratio_*top, aspect_ratio_*top, -top, top, znear_, zfar_});
	return true;
}

bool FreeflyController::on_mouse_button_event(const MouseButtonEvent& event)
{
	return false;
}

bool FreeflyController::on_mouse_moved_event(const MouseMovedEvent& event)
{
	if(!has_control_)
		return false;

	// Update and constrain yaw and pitch
	float old_x = win_x_+0.5f*win_width_;
	float old_y = win_y_+0.5f*win_height_;
	camera_yaw_   -= camera_rotation_speed_ * (event.x-old_x);
	camera_yaw_    = (camera_yaw_>360.f) ? camera_yaw_-360.f : camera_yaw_;
	camera_yaw_    = (camera_yaw_<0.f)   ? 360.f-camera_yaw_ : camera_yaw_;
	camera_pitch_ -= camera_rotation_speed_ * (event.y-old_y);
	camera_pitch_  = (camera_pitch_> 89.f) ?  89.f : camera_pitch_;
	camera_pitch_  = (camera_pitch_<-89.f) ? -89.f : camera_pitch_;

	// Set cursor back to the center of the screen
	Input::set_mouse_position(win_x_+0.5f*win_width_, win_y_+0.5f*win_height_);
	// Input::center_mouse_position();

	return true;
}


} // namespace editor