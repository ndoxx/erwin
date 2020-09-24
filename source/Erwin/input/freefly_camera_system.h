#pragma once

#include "entity/component/camera.h"

namespace erwin
{

class GameClock;
struct ComponentTransform3D;
struct WindowResizeEvent;
struct WindowMovedEvent;
struct MouseScrollEvent;
struct MouseMovedEvent;
struct MouseButtonEvent;
struct KeyboardEvent;

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
	void init(const ComponentTransform3D& transform);
	void set_frustum_parameters(const FrustumParameters& params);
	void update_frustum();
	void update(const GameClock& clock, ComponentCamera3D& camera, ComponentTransform3D& transform);
	void transfer_control(bool value);

	bool on_window_resize_event(const WindowResizeEvent& event);
	bool on_window_moved_event(const WindowMovedEvent& event);
	bool on_mouse_scroll_event(const MouseScrollEvent& event);
	bool on_mouse_moved_event(const MouseMovedEvent& event);
	bool on_mouse_button_event(const MouseButtonEvent& event);
	bool on_keyboard_event(const KeyboardEvent& event);

	inline float get_aspect_ratio() const { return frustum_parameters_.aspect_ratio; }
	inline float get_fovy() const         { return frustum_parameters_.fovy; }
	inline float get_znear() const        { return frustum_parameters_.znear; }
	inline float get_zfar() const         { return frustum_parameters_.zfar; }

private:
	FrustumParameters frustum_parameters_;
	ComponentCamera3D::Frustum3D frustum_;

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