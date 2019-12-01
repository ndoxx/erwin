#include "input/camera_2d_controller.h"
#include "input/input.h"
#include "debug/logger.h"

namespace erwin
{

using namespace keymap;

OrthographicCamera2DController::OrthographicCamera2DController(float aspect_ratio, float zoom_level):
camera_({-aspect_ratio*zoom_level, aspect_ratio*zoom_level, -zoom_level, zoom_level}),
aspect_ratio_(aspect_ratio),
zoom_level_(zoom_level),
camera_translation_speed_(1.f),
camera_rotation_speed_(50.f),
camera_angle_(camera_.get_angle()),
camera_position_(camera_.get_position())
{

}

OrthographicCamera2DController::~OrthographicCamera2DController()
{
	
}

void OrthographicCamera2DController::update(GameClock& clock)
{
	// * Handle inputs
	float dt = clock.get_frame_duration();

	// Translation
	float speed_modifier = Input::is_key_pressed(WKEY::LEFT_SHIFT) ? 3.f : 1.f;

	if(Input::is_key_pressed(WKEY::W)) // UP
		camera_position_ += dt*speed_modifier*camera_translation_speed_*camera_.get_up();
	if(Input::is_key_pressed(WKEY::S)) // DOWN
		camera_position_ -= dt*speed_modifier*camera_translation_speed_*camera_.get_up();
	if(Input::is_key_pressed(WKEY::A)) // LEFT
		camera_position_ -= dt*speed_modifier*camera_translation_speed_*camera_.get_right();
	if(Input::is_key_pressed(WKEY::D)) // RIGHT
		camera_position_ += dt*speed_modifier*camera_translation_speed_*camera_.get_right();

	// Rotation
	if(Input::is_key_pressed(WKEY::E)) // ROTATE CCW
	{
		camera_angle_ += dt*camera_rotation_speed_;
		if(camera_angle_>360.f)
			camera_angle_ -= 360.f;
	}
	if(Input::is_key_pressed(WKEY::Q)) // ROTATE CW
	{
		camera_angle_ -= dt*camera_rotation_speed_;
		if(camera_angle_<0.f)
			camera_angle_ += 360.f;
	}
	if(Input::is_key_pressed(WKEY::R)) // RESET CAMERA ANGLE
		camera_angle_ = 0.f;

	// * Update camera
	// camera_.set_position(camera_position_);
	// camera_.set_angle(camera_angle_);
	camera_.set_parameters(camera_position_, camera_angle_);
}

bool OrthographicCamera2DController::on_window_resize_event(const WindowResizeEvent& event)
{
	aspect_ratio_ = event.width/float(event.height);
	camera_.set_projection({-aspect_ratio_*zoom_level_, aspect_ratio_*zoom_level_, -zoom_level_, zoom_level_});
	return false;
}

bool OrthographicCamera2DController::on_mouse_scroll_event(const MouseScrollEvent& event)
{
	if(event.y_offset<0)
		zoom_level_ *= 1.05f;
	else
		zoom_level_ *= 0.95f;
	camera_translation_speed_ = zoom_level_;
	camera_.set_projection({-aspect_ratio_*zoom_level_, aspect_ratio_*zoom_level_, -zoom_level_, zoom_level_});
	return false;
}


} // namespace erwin