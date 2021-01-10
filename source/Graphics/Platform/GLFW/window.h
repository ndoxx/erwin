#pragma once

#include "../../window.h"

struct GLFWwindow;

namespace gfx
{

class GLFWWindow : public Window
{
public:
	GLFWWindow(DeviceAPI api, const WindowProps& props);
	virtual ~GLFWWindow();

    void set_title(const std::string& value) override;
    void set_vsync(bool value) override;
    void* get_native() const override;
    void poll_events() const override;
    void make_current() const override;

    void set_window_close_callback(WindowCloseCallback cb) override;
    void set_window_resize_callback(WindowResizeCallback cb) override;
    void set_framebuffer_resize_callback(FramebufferResizeCallback cb) override;
    void set_keyboard_callback(KeyboardCallback cb) override;
    void set_key_typed_callback(KeyTypedCallback cb) override;
    void set_mouse_button_callback(MouseButtonCallback cb) override;
    void set_mouse_move_callback(MouseMoveCallback cb) override;
    void set_mouse_scroll_callback(MouseScrollCallback cb) override;

private:
	WindowCloseCallback window_close_cb_;
	WindowResizeCallback window_resize_cb_;
	FramebufferResizeCallback framebuffer_resize_cb_;
	KeyboardCallback keyboard_cb_;
	KeyTypedCallback key_typed_cb_;
	MouseButtonCallback mouse_button_cb_;
	MouseMoveCallback mouse_move_cb_;
	MouseScrollCallback mouse_scroll_cb_;

	GLFWwindow* handle_;
	static uint8_t s_num_windows;
};

}