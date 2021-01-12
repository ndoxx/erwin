#include "Platform/GLFW/glfw_window.h"
#include "GLFW/glfw3.h"
#include <kibble/logger/logger.h>

namespace gfx
{

uint8_t GLFWWindow::s_num_windows = 0;

GLFWWindow::GLFWWindow(const WindowProps& props) : Window(props)
{
    // Initialize GLFW if not already initialized
    if(s_num_windows == 0)
    {
        KLOGN("core") << "Initializing GLFW." << std::endl;
        if(!glfwInit())
        {
            KLOGF("core") << "Failed to initialize GLFW." << std::endl;
            exit(0);
        }

        KLOG("core", 0) << "[GLFW]" << std::endl;
        // Show GLFW version
        {
            int major, minor, rev;
            glfwGetVersion(&major, &minor, &rev);
            KLOGI << "Version:  " << major << "." << minor << "." << rev << std::endl;
        }
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, props.resizable ? GLFW_TRUE : GLFW_FALSE);

    if(props.api == DeviceAPI::Vulkan)
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Open a window
    if(props_.full_screen)
        handle_ = glfwCreateWindow(int(props_.width), int(props_.height), props_.title.c_str(), glfwGetPrimaryMonitor(),
                                   nullptr);
    else
        handle_ = glfwCreateWindow(int(props_.width), int(props_.height), props_.title.c_str(), nullptr, nullptr);
    ++s_num_windows;

    if(!handle_)
    {
        KLOGF("core") << "Failed to open GLFW window." << std::endl;
        exit(0);
    }

    // Make window always-on-top if required
    if(!props_.full_screen && props_.always_on_top)
        glfwSetWindowAttrib(handle_, GLFW_FLOATING, GLFW_TRUE);

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(handle_, GLFW_STICKY_KEYS, GL_TRUE);

    // VSync
    glfwSwapInterval(props_.vsync ? 1 : 0);

    // Send GLFW a raw pointer to our data structure so we can act on it from callbacks
    glfwSetWindowUserPointer(handle_, this);

    // Set error callback
    glfwSetErrorCallback([](int error_code, const char* description) {
        KLOGE("core") << "GLFW Error (" << error_code << "): " << description << std::endl;
    });
}
GLFWWindow::~GLFWWindow()
{
    glfwDestroyWindow(handle_);
    if(--s_num_windows == 0)
    {
        KLOGN("core") << "Shutting down GLFW." << std::endl;
        glfwTerminate();
    }
}

void GLFWWindow::set_title(const std::string& value)
{
    props_.title = value;
    glfwSetWindowTitle(handle_, props_.title.c_str());
}

void GLFWWindow::set_vsync(bool value)
{
    props_.vsync = value;
    glfwSwapInterval(value ? 1 : 0);
}

void* GLFWWindow::get_native() const { return static_cast<void*>(handle_); }

void GLFWWindow::poll_events() const { glfwPollEvents(); }

void GLFWWindow::make_current() const
{
    if(glfwGetCurrentContext() != handle_)
        glfwMakeContextCurrent(handle_);
}

void GLFWWindow::set_window_close_callback(WindowCloseCallback cb)
{
    window_close_cb_ = cb;
    glfwSetWindowCloseCallback(handle_, [](auto pwnd) {
        auto* pw = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(pwnd));
        pw->window_close_cb_();
    });
}

void GLFWWindow::set_window_resize_callback(WindowResizeCallback cb)
{
    window_resize_cb_ = cb;
    // If we render to a GUI window for example, we rather react to the GUI window resize events.
    // So we don't set this callback in that case.
    if(props_.host)
    {
        glfwSetWindowSizeCallback(handle_, [](GLFWwindow* pwnd, int width, int height) {
            auto* pw = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(pwnd));
            pw->window_resize_cb_(uint32_t(width), uint32_t(height));
        });
    }
}

void GLFWWindow::set_framebuffer_resize_callback(FramebufferResizeCallback cb)
{
    framebuffer_resize_cb_ = cb;
    // If we render to a GUI window for example, we rather react to the GUI window resize events.
    // So we don't set this callback in that case.
    if(props_.host)
    {
        glfwSetFramebufferSizeCallback(handle_, [](GLFWwindow* pwnd, int width, int height) {
            auto* pw = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(pwnd));
            pw->framebuffer_resize_cb_(uint32_t(width), uint32_t(height));
        });
    }
}

void GLFWWindow::set_keyboard_callback(KeyboardCallback cb)
{
    keyboard_cb_ = cb;
    glfwSetKeyCallback(handle_, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto* pw = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        pw->keyboard_cb_(key, scancode, action, mods);
    });
}

void GLFWWindow::set_key_typed_callback(KeyTypedCallback cb)
{
    key_typed_cb_ = cb;
    glfwSetCharCallback(handle_, [](GLFWwindow* window, unsigned int codepoint) {
        auto* pw = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        pw->key_typed_cb_(codepoint);
    });
}

void GLFWWindow::set_mouse_button_callback(MouseButtonCallback cb)
{
    mouse_button_cb_ = cb;
    glfwSetMouseButtonCallback(handle_, [](GLFWwindow* window, int button, int action, int mods) {
        auto* pw = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        pw->mouse_button_cb_(button, action, mods);
    });
}

void GLFWWindow::set_mouse_move_callback(MouseMoveCallback cb)
{
    mouse_move_cb_ = cb;
    glfwSetCursorPosCallback(handle_, [](GLFWwindow* window, double x, double y) {
        auto* pw = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        pw->mouse_move_cb_(float(x), float(y));
    });
}

void GLFWWindow::set_mouse_scroll_callback(MouseScrollCallback cb)
{
    mouse_scroll_cb_ = cb;
    glfwSetScrollCallback(handle_, [](GLFWwindow* window, double x_offset, double y_offset) {
        auto* pw = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        pw->mouse_scroll_cb_(float(x_offset), float(y_offset));
    });
}

} // namespace gfx