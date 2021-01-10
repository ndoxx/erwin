#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <memory>

namespace gfx
{

struct WindowProps
{
    std::string title = "ErwinEngine";
    uint32_t width = 1280;
    uint32_t height = 1024;
    bool full_screen = false;
    bool always_on_top = false;
    bool vsync = true;
    bool host = true;
};

// Abstract class for window handling
class Window
{
public:
    Window(const WindowProps& props) : props_(props) {}
    virtual ~Window() = default;

    // Get window title
    inline const std::string& get_title() const { return props_.title; }
    // Get window width in pixels
    inline uint32_t get_width() const { return props_.width; }
    // Get window height in pixels
    inline uint32_t get_height() const { return props_.height; }
    // Is VSync enabled
    inline bool get_vsync() { return props_.vsync; }
    // Modify the window title
    virtual void set_title(const std::string& value) = 0;
    // Enable / Disable VSync
    virtual void set_vsync(bool value) = 0;
    // Return native window pointer
    virtual void* get_native() const = 0;
    // Swap buffers for image presentation
    virtual void swap_buffers() const = 0;
    // Poll window events. Call after buffer swap during update.
    virtual void poll_events() const = 0;
    // Make context current
    virtual void make_current() const = 0;

    // Window event callback types
    using WindowCloseCallback = std::function<void()>;
	using WindowResizeCallback = std::function<void(uint32_t w, uint32_t h)>;
	using FramebufferResizeCallback = std::function<void(uint32_t w, uint32_t h)>;
	using KeyboardCallback = std::function<void(int32_t key, int32_t scancode, int32_t action, int32_t mods)>;
	using KeyTypedCallback = std::function<void(uint32_t codepoint)>;
	using MouseButtonCallback = std::function<void(int32_t button, int32_t action, int32_t mods)>;
	using MouseMoveCallback = std::function<void(float x, float y)>;
	using MouseScrollCallback = std::function<void(float offset_x, float offset_y)>;

	// Setters for event callbacks
    virtual void set_window_close_callback(WindowCloseCallback cb) = 0;
    virtual void set_window_resize_callback(WindowResizeCallback cb) = 0;
    virtual void set_framebuffer_resize_callback(FramebufferResizeCallback cb) = 0;
    virtual void set_keyboard_callback(KeyboardCallback cb) = 0;
    virtual void set_key_typed_callback(KeyTypedCallback cb) = 0;
    virtual void set_mouse_button_callback(MouseButtonCallback cb) = 0;
    virtual void set_mouse_move_callback(MouseMoveCallback cb) = 0;
    virtual void set_mouse_scroll_callback(MouseScrollCallback cb) = 0;

    // Factory method to construct a concrete window type
    // Only one implementation exists at compile-time (ensured by the build system)
	static std::unique_ptr<Window> create(const WindowProps& props = {});

protected:
    WindowProps props_;
};

} // namespace gfx