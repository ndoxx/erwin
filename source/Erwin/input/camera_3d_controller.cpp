#include "input/camera_3d_controller.h"
#include "input/input.h"
#include "debug/logger.h"

namespace erwin
{

using namespace keymap;

inline float fovy_znear_to_top(float fovy, float znear)
{
	return znear * tan(0.5f * fovy * (M_PI / 180.f));
}

PerspectiveFreeflyController::PerspectiveFreeflyController(float aspect_ratio, float fovy, float znear, float zfar):
camera_({-aspect_ratio*fovy_znear_to_top(fovy,znear), aspect_ratio*fovy_znear_to_top(fovy,znear),
         -fovy_znear_to_top(fovy,znear), fovy_znear_to_top(fovy,znear),
         znear, zfar}),
aspect_ratio_(aspect_ratio),
fovy_(fovy),
znear_(znear),
zfar_(zfar),
camera_translation_speed_(2.f),
camera_rotation_speed_(2.f * M_PI / 180.f),
camera_yaw_(camera_.get_yaw()),
camera_pitch_(camera_.get_pitch()),
camera_position_(camera_.get_position())
{
	std::tie(last_mouse_x_, last_mouse_y_) = Input::get_mouse_position();
}

PerspectiveFreeflyController::~PerspectiveFreeflyController()
{
	
}

void PerspectiveFreeflyController::update(GameClock& clock)
{
	// * Handle inputs
	float dt = clock.get_frame_duration();

	// Translation
	float speed_modifier = Input::is_key_pressed(WKEY::LEFT_SHIFT) ? 3.f : 1.f;

	// Front direction
	glm::vec3 front = camera_.get_forward();
	front.y = 0.f;
	glm::normalize(front);

	if(Input::is_key_pressed(WKEY::W)) // FORWARD
		camera_position_ -= dt*speed_modifier*camera_translation_speed_*front;
	if(Input::is_key_pressed(WKEY::S)) // BACKWARD
		camera_position_ += dt*speed_modifier*camera_translation_speed_*front;
	if(Input::is_key_pressed(WKEY::A)) // LEFT
		camera_position_ -= dt*speed_modifier*camera_translation_speed_*camera_.get_right();
	if(Input::is_key_pressed(WKEY::D)) // RIGHT
		camera_position_ += dt*speed_modifier*camera_translation_speed_*camera_.get_right();
	if(Input::is_key_pressed(WKEY::SPACE)) // UP
		camera_position_ += dt*speed_modifier*camera_translation_speed_*glm::vec3(0.f,1.f,0.f);
	if(Input::is_key_pressed(WKEY::LEFT_CONTROL)) // DOWN
		camera_position_ -= dt*speed_modifier*camera_translation_speed_*glm::vec3(0.f,1.f,0.f);

	camera_.set_parameters(camera_position_, camera_yaw_, camera_pitch_);
}

bool PerspectiveFreeflyController::on_window_resize_event(const WindowResizeEvent& event)
{
	aspect_ratio_ = event.width/float(event.height);
	float top = fovy_znear_to_top(fovy_, znear_);
	camera_.set_projection({-aspect_ratio_*top, aspect_ratio_*top, -top, top, znear_, zfar_});
	return false;
}

bool PerspectiveFreeflyController::on_mouse_scroll_event(const MouseScrollEvent& event)
{
	if(event.y_offset<0)
		fovy_ *= 1.05f;
	else
		fovy_ *= 0.95f;
	// camera_translation_speed_ = ;
	float top = fovy_znear_to_top(fovy_, znear_);
	camera_.set_projection({-aspect_ratio_*top, aspect_ratio_*top, -top, top, znear_, zfar_});
	return false;
}

bool PerspectiveFreeflyController::on_mouse_moved_event(const MouseMovedEvent& event)
{
	if(!Input::is_mouse_button_pressed(WMOUSE::BUTTON_0))
	{
		last_mouse_x_ = event.x;
		last_mouse_y_ = event.y;
		return false;
	}

	float dx = event.x - last_mouse_x_;
	float dy = event.y - last_mouse_y_;

	camera_yaw_   -= camera_rotation_speed_ * dx;
	camera_pitch_ += camera_rotation_speed_ * dy;

	last_mouse_x_ = event.x;
	last_mouse_y_ = event.y;

	return false;
}


} // namespace erwin