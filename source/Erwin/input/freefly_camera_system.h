#pragma once

#include <optional>
#include "entity/component_camera.h"
#include "entity/component_transform.h"
#include "core/game_clock.h"
#include "event/window_events.h"
#include "entity/reflection.h"

namespace erwin
{

class FreeflyCameraSystem
{
public:
	struct FrustumParameters
	{
		float aspect_ratio = 1280.f / 1024.f;
		float fovy = 60.f;
		float znear = 0.1f;
		float zfar = 100.f;
	};

	FreeflyCameraSystem();
	void init(ComponentCamera3D& camera, ComponentTransform3D& transform);
	void set_frustum_parameters(const FrustumParameters& params);
	void update(const erwin::GameClock& clock);

	bool on_window_resize_event(const erwin::WindowResizeEvent& event);
	bool on_window_moved_event(const erwin::WindowMovedEvent& event);
	bool on_mouse_scroll_event(const erwin::MouseScrollEvent& event);
	bool on_mouse_moved_event(const erwin::MouseMovedEvent& event);
	bool on_mouse_button_event(const erwin::MouseButtonEvent& event);
	bool on_keyboard_event(const erwin::KeyboardEvent& event);

	inline float get_aspect_ratio() const { return frustum_parameters_.aspect_ratio; }
	inline float get_fovy() const         { return frustum_parameters_.fovy; }
	inline float get_znear() const        { return frustum_parameters_.znear; }
	inline float get_zfar() const         { return frustum_parameters_.zfar; }

private:
	void update_frustum();

private:
	FrustumParameters frustum_parameters_;
	ComponentCamera3D::Frustum3D frustum_;

	std::optional<std::reference_wrapper<ComponentCamera3D>> target_camera_;
	std::optional<std::reference_wrapper<ComponentTransform3D>> target_transform_;

	float translation_speed_;
	float rotation_speed_;
	float yaw_;
	float pitch_;
	float win_width_;
	float win_height_;
	float win_x_;
	float win_y_;
	float prev_mouse_x_;
	float prev_mouse_y_;
	bool inputs_enabled_;
	bool dirty_frustum_;
	glm::vec3 position_;
};


} // namespace erwin