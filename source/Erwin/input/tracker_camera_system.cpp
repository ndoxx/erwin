#include <cmath>

#include "input/input.h"
#include "input/tracker_camera_system.h"

#include "debug/logger.h"

namespace erwin
{

// Helper function to get "top" dimension from FOV and z-near
inline float fovy_znear_to_top(float fovy, float znear)
{
    return znear * std::tan(0.5f * fovy * (float(M_PI) / 180.f));
}

TrackerCameraSystem::TrackerCameraSystem()
    : rotation_speed_(2.5f * float(M_PI) / 180.f), azimuth_(0.f),
      colatitude_(90.f), radius_(5.f), win_width_(0.f), win_height_(0.f), win_x_(0.f), win_y_(0.f), prev_mouse_x_(0.f),
      prev_mouse_y_(0.f), inputs_enabled_(false), dirty_frustum_(true), position_(0.f), lookat_target_(0.f)
{}

void TrackerCameraSystem::init(ComponentCamera3D& camera, ComponentTransform3D& transform)
{
    target_camera_ = camera;
    target_transform_ = transform;

    position_ = transform.position;
    set_position(radius_, azimuth_, colatitude_);
}

void TrackerCameraSystem::set_frustum_parameters(const FrustumParameters& params)
{
    frustum_parameters_ = params;
    update_frustum();
}

void TrackerCameraSystem::update_frustum()
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

void TrackerCameraSystem::set_position(float radius, float azimuth, float colatitude)
{
    radius_ = radius;
    azimuth_ = azimuth;
    colatitude_ = colatitude;

    azimuth_ = (azimuth_ > 360.f) ? azimuth_ - 360.f : azimuth_;
    azimuth_ = (azimuth_ < 0.f) ? 360.f - azimuth_ : azimuth_;
    colatitude_ = (colatitude_ > 179.f) ? 179.f : colatitude_;
    colatitude_ = (colatitude_ < 1.f) ? 1.f : colatitude_;

    position_ = {radius_ * std::sin(glm::radians(colatitude_)) * std::cos(glm::radians(azimuth_)),
                 radius_ * std::cos(glm::radians(colatitude_)),
                 radius_ * std::sin(glm::radians(colatitude_)) * std::sin(glm::radians(azimuth_))};
}

void TrackerCameraSystem::update(const erwin::GameClock& /*clock*/)
{
    // Update camera projection
    if(dirty_frustum_)
        target_camera_->get().set_projection(frustum_);

    if(!inputs_enabled_ && !dirty_frustum_)
        return;

    ComponentCamera3D& camera = target_camera_->get();
    ComponentTransform3D& transform = target_transform_->get();

    transform.position = position_;
    camera.view_matrix = glm::lookAt(position_, lookat_target_, glm::vec3(0.0f, 1.0f, 0.0f));
    camera.view_projection_matrix = camera.projection_matrix * camera.view_matrix;
    // TMP: camera.update_transform() not called, so world frustum and directions stay uninit, also transform angles are uninit

    if(dirty_frustum_)
        dirty_frustum_ = false;
}

bool TrackerCameraSystem::on_window_resize_event(const WindowResizeEvent& event)
{
    win_width_ = float(event.width);
    win_height_ = float(event.height);
    frustum_parameters_.aspect_ratio = float(event.width) / float(event.height);
    update_frustum();
    return false;
}

bool TrackerCameraSystem::on_window_moved_event(const erwin::WindowMovedEvent& event)
{
    win_x_ = float(event.x);
    win_y_ = float(event.y);
    return false;
}

bool TrackerCameraSystem::on_mouse_scroll_event(const MouseScrollEvent& event)
{
    if(!inputs_enabled_)
        return false;

    // Update radius
    float factor = (event.y_offset < 0) ? 1.05f : 0.95f;
    set_position(radius_ * factor, azimuth_, colatitude_);

    return true;
}

bool TrackerCameraSystem::on_mouse_button_event(const MouseButtonEvent&) { return false; }

bool TrackerCameraSystem::on_mouse_moved_event(const MouseMovedEvent& event)
{
    if(!inputs_enabled_)
        return false;

    float dphi = rotation_speed_ * float(event.x - (win_x_ + 0.5f * win_width_));
    float dtheta = rotation_speed_ * float(event.y - (win_y_ + 0.5f * win_height_));

    set_position(radius_, azimuth_ - dphi, colatitude_ + dtheta);

    // Set cursor back to the center of the screen
    Input::set_mouse_position(win_x_ + 0.5f * win_width_, win_y_ + 0.5f * win_height_);
    // Input::center_mouse_position();

    return true;
}

bool TrackerCameraSystem::on_keyboard_event(const erwin::KeyboardEvent& event)
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