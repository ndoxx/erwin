#include <cmath>

#include "input/freefly_camera_system.h"
#include "input/input.h"
#include "core/game_clock.h"
#include "event/window_events.h"
#include "entity/component_transform.h"
#include "entity/reflection.h"

#include "debug/logger.h"

namespace erwin
{

// Helper function to get "top" dimension from FOV and z-near
inline float fovy_znear_to_top(float fovy, float znear)
{
    return znear * std::tan(0.5f * fovy * (float(M_PI) / 180.f));
}

FreeflyCameraSystem::FreeflyCameraSystem()
    : translation_speed_(2.f), rotation_speed_(2.5f * float(M_PI) / 180.f),
      yaw_(0.f), pitch_(0.f), win_width_(0.f), win_height_(0.f), win_x_(0.f), win_y_(0.f), prev_mouse_x_(0.f),
      prev_mouse_y_(0.f), inputs_enabled_(false), dirty_frustum_(true), position_(0.f)
{}

void FreeflyCameraSystem::init(ComponentTransform3D& transform)
{
    position_ = transform.position;
    pitch_ = transform.euler.x;
    yaw_ = transform.euler.y;
}

void FreeflyCameraSystem::set_frustum_parameters(const FrustumParameters& params)
{
    frustum_parameters_ = params;
    update_frustum();
}

void FreeflyCameraSystem::update_frustum()
{
    float top = fovy_znear_to_top(frustum_parameters_.fovy, frustum_parameters_.znear);
    frustum_ = {-frustum_parameters_.aspect_ratio * top,
                frustum_parameters_.aspect_ratio * top,
                -top,
                top,
                frustum_parameters_.znear, // TODO: minus?
                frustum_parameters_.zfar};
    dirty_frustum_ = true;
}

void FreeflyCameraSystem::update(const erwin::GameClock& clock, ComponentCamera3D& camera, ComponentTransform3D& transform)
{
    // Update camera projection
    if(dirty_frustum_)
        camera.set_projection(frustum_);

    if(!inputs_enabled_ && !dirty_frustum_)
        return;

    // Translational magnitude
    float dt = clock.get_frame_duration();
    float speed_modifier = Input::is_action_key_pressed(ACTION_FREEFLY_GO_FAST) ? 5.f : 1.f;
    float translation = dt * speed_modifier * translation_speed_;

    // Front direction is the normalized projection of the camera forward axis on the horizontal plane
    glm::vec3 front = camera.forward;
    front.y = 0.f;
    front = glm::normalize(front);

    // Handle keyboard inputs
    if(Input::is_action_key_pressed(ACTION_FREEFLY_MOVE_FORWARD))
        position_ += translation * front;
    if(Input::is_action_key_pressed(ACTION_FREEFLY_MOVE_BACKWARD))
        position_ -= translation * front;
    if(Input::is_action_key_pressed(ACTION_FREEFLY_STRAFE_LEFT))
        position_ -= translation * camera.right;
    if(Input::is_action_key_pressed(ACTION_FREEFLY_STRAFE_RIGHT))
        position_ += translation * camera.right;
    if(Input::is_action_key_pressed(ACTION_FREEFLY_ASCEND))
        position_ += translation * glm::vec3(0.f, 1.f, 0.f);
    if(Input::is_action_key_pressed(ACTION_FREEFLY_DESCEND))
        position_ -= translation * glm::vec3(0.f, 1.f, 0.f);

    // Update components
    transform.position = position_;
    transform.set_rotation({pitch_, yaw_, 0.f});
    camera.update_transform(transform.get_model_matrix());

    if(dirty_frustum_)
        dirty_frustum_ = false;
}

bool FreeflyCameraSystem::on_window_resize_event(const WindowResizeEvent& event)
{
    win_width_ = float(event.width);
    win_height_ = float(event.height);
    frustum_parameters_.aspect_ratio = float(event.width) / float(event.height);
    update_frustum();
    return false;
}

bool FreeflyCameraSystem::on_window_moved_event(const erwin::WindowMovedEvent& event)
{
    win_x_ = float(event.x);
    win_y_ = float(event.y);
    return false;
}

bool FreeflyCameraSystem::on_mouse_scroll_event(const MouseScrollEvent& event)
{
    if(!inputs_enabled_)
        return false;

    // Update and constrain FOV
    frustum_parameters_.fovy *= (event.y_offset < 0) ? 1.05f : 0.95f;
    frustum_parameters_.fovy = (frustum_parameters_.fovy > 120.f) ? 120.f : frustum_parameters_.fovy;
    frustum_parameters_.fovy = (frustum_parameters_.fovy < 20.f) ? 20.f : frustum_parameters_.fovy;
    update_frustum();
    return true;
}

bool FreeflyCameraSystem::on_mouse_button_event(const MouseButtonEvent&) { return false; }

bool FreeflyCameraSystem::on_mouse_moved_event(const MouseMovedEvent& event)
{
    if(!inputs_enabled_)
        return false;

    // Update and constrain yaw and pitch
    float dx = float(event.x - (win_x_ + 0.5f * win_width_));
    float dy = float(event.y - (win_y_ + 0.5f * win_height_));
    yaw_ -= rotation_speed_ * dx;
    yaw_ = (yaw_ > 360.f) ? yaw_ - 360.f : yaw_;
    yaw_ = (yaw_ < 0.f) ? 360.f - yaw_ : yaw_;
    pitch_ -= rotation_speed_ * dy;
    pitch_ = (pitch_ > 89.f) ? 89.f : pitch_;
    pitch_ = (pitch_ < -89.f) ? -89.f : pitch_;

    // Set cursor back to the center of the screen
    Input::set_mouse_position(win_x_ + 0.5f * win_width_, win_y_ + 0.5f * win_height_);
    // Input::center_mouse_position();

    return true;
}

bool FreeflyCameraSystem::on_keyboard_event(const erwin::KeyboardEvent& event)
{
    if(Input::match_action(ACTION_FREEFLY_TOGGLE, event))
    {
        // Toggle inputs
        inputs_enabled_ = !inputs_enabled_;
        Input::show_cursor(!inputs_enabled_);

        // Save mouse position if control was acquired, restore it if control was released
        if(inputs_enabled_)
        {
            std::tie(prev_mouse_x_, prev_mouse_y_) = Input::get_mouse_position();
            Input::set_mouse_position(win_x_ + 0.5f * win_width_, win_y_ + 0.5f * win_height_);
        }
        else
            Input::set_mouse_position(prev_mouse_x_, prev_mouse_y_);

        return true;
    }

    return false;
}

} // namespace erwin