#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "glfw_keymap.h"
#include "glfw_window.h"
#include "ogl_context.h"

#include "../Erwin/core/core.h"
#include <kibble/logger/logger.h>

#include "../Erwin/event/event_bus.h"
#include "../Erwin/event/window_events.h"

namespace erwin
{
/*
#ifndef NDEBUG
void APIENTRY glDebugOutput(GLenum source,
                            GLenum type,
                            GLuint id,
                            GLenum severity,
                            GLsizei length,
                            const GLchar *message,
                            const void *userParam)
{
    // ignore non-significant error/warning codes
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    KLOGR("render") << "---------------" << std::endl;
    KLOGE("render") << "Debug message (" << id << "): " <<  message << std::endl;

    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             { KLOGI << "Source: API" << std::endl; } break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   { KLOGI << "Source: Window System" << std::endl; } break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: { KLOGI << "Source: Shader Compiler" << std::endl; } break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     { KLOGI << "Source: Third Party" << std::endl; } break;
        case GL_DEBUG_SOURCE_APPLICATION:     { KLOGI << "Source: Application" << std::endl; } break;
        case GL_DEBUG_SOURCE_OTHER:           { KLOGI << "Source: Other" << std::endl; } break;
    }

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               { KLOGI << "Type: Error" << std::endl; } break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: { KLOGI << "Type: Deprecated Behaviour" << std::endl; } break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  { KLOGI << "Type: Undefined Behaviour" << std::endl; } break;
        case GL_DEBUG_TYPE_PORTABILITY:         { KLOGI << "Type: Portability" << std::endl; } break;
        case GL_DEBUG_TYPE_PERFORMANCE:         { KLOGI << "Type: Performance" << std::endl; } break;
        case GL_DEBUG_TYPE_MARKER:              { KLOGI << "Type: Marker" << std::endl; } break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          { KLOGI << "Type: Push Group" << std::endl; } break;
        case GL_DEBUG_TYPE_POP_GROUP:           { KLOGI << "Type: Pop Group" << std::endl; } break;
        case GL_DEBUG_TYPE_OTHER:               { KLOGI << "Type: Other" << std::endl; } break;
    }

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         { KLOGI << "Severity: high" << std::endl; } break;
        case GL_DEBUG_SEVERITY_MEDIUM:       { KLOGI << "Severity: medium" << std::endl; } break;
        case GL_DEBUG_SEVERITY_LOW:          { KLOGI << "Severity: low" << std::endl; } break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: { KLOGI << "Severity: notification" << std::endl; } break;
    }
}
#endif
*/
static uint8_t s_glfw_num_windows = 0;

WScope<Window> Window::create(EventBus& event_bus, const WindowProps& props) { return make_scope<GLFWWindow>(props, event_bus); }

static void GLFW_error_callback(int error, const char* description)
{
    KLOGE("core") << "GLFW Error (" << error << "): " << description << std::endl;
}

struct GLFWWindow::GLFWWindowDataImpl
{
    GLFWwindow* window;
    EventBus& event_bus_;
    bool vsync;
    int width;
    int height;
    std::string title;

    GLFWWindowDataImpl(EventBus& event_bus) : event_bus_(event_bus) {}

    inline void on_close() const { event_bus_.enqueue(WindowCloseEvent()); }
    inline void on_window_resize(int w, int h)
    {
        width = w;
        height = h;
        event_bus_.enqueue(WindowResizeEvent(width, height));
    }
    inline void on_framebuffer_resize(int w, int h) const { event_bus_.enqueue(FramebufferResizeEvent(w, h)); }
    inline void on_keyboard(int key, int /*scancode*/, int action, int mods) const
    {
        keymap::WKEY wkey = keymap::GLFW_KEY_to_WKEY(uint16_t(key));
        switch(action)
        {
        case GLFW_PRESS: {
            event_bus_.enqueue(KeyboardEvent(wkey, uint8_t(mods), true, false));
            break;
        }
        case GLFW_RELEASE: {
            event_bus_.enqueue(KeyboardEvent(wkey, uint8_t(mods), false, false));
            break;
        }
        case GLFW_REPEAT: {
            event_bus_.enqueue(KeyboardEvent(wkey, uint8_t(mods), true, true));
            break;
        }
        }
    }
    inline void on_key_typed(unsigned int codepoint) const { event_bus_.enqueue(KeyTypedEvent(codepoint)); }
    inline void on_mouse_button(int button, int action, int mods) const
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        keymap::WMOUSE wbutton = keymap::GLFW_MB_to_WMOUSE(uint16_t(button));

        switch(action)
        {
        case GLFW_PRESS: {
            event_bus_.enqueue(MouseButtonEvent(wbutton, uint8_t(mods), true, float(x), float(y)));
            break;
        }
        case GLFW_RELEASE: {
            event_bus_.enqueue(MouseButtonEvent(wbutton, uint8_t(mods), false, float(x), float(y)));
            break;
        }
        }
    }
    inline void on_mouse_move(double x, double y) const { event_bus_.enqueue(MouseMovedEvent(float(x), float(y))); }
    inline void on_mouse_scroll(double x_offset, double y_offset) const
    {
        event_bus_.enqueue(MouseScrollEvent(float(x_offset), float(y_offset)));
    }
};

GLFWWindow::GLFWWindow(const WindowProps& props, EventBus& event_bus)
    : data_(std::make_unique<GLFWWindowDataImpl>(event_bus))
{
    init(props);
}

GLFWWindow::~GLFWWindow() { cleanup(); }

void GLFWWindow::init(const WindowProps& props)
{
    W_PROFILE_FUNCTION()

    // Initialize GLFW if not already initialized
    if(s_glfw_num_windows == 0)
    {
        KLOGN("render") << "Initializing GLFW." << std::endl;
        if(!glfwInit())
        {
            KLOGF("render") << "Failed to initialize GLFW." << std::endl;
            fatal();
        }

        KLOG("render", 0) << "[GLFW]" << std::endl;
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
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    /*
        // Debug context if we run a debug build
    #ifndef NDEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    #endif
    */
    // Open a window
    if(props.full_screen)
        data_->window =
            glfwCreateWindow(int(props.width), int(props.height), props.title.c_str(), glfwGetPrimaryMonitor(), NULL);
    else
        data_->window = glfwCreateWindow(int(props.width), int(props.height), props.title.c_str(), NULL, NULL);
    ++s_glfw_num_windows;

    data_->width = int(props.width);
    data_->height = int(props.height);
    data_->title = props.title;

    if(data_->window == NULL)
    {
        KLOGF("core") << "Failed to open GLFW window." << std::endl;
        fatal();
    }
    /*
        // Init debug output
    #ifndef NDEBUG
        GLint flags;
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if(flags & GL_CONTEXT_FLAG_DEBUG_BIT)
        {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(glDebugOutput, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        }
    #endif
    */

    // Make window always-on-top if required
    if(!props.full_screen && props.always_on_top)
        glfwSetWindowAttrib(data_->window, GLFW_FLOATING, GLFW_TRUE);

    context_ = new OGLContext(data_->window);

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(data_->window, GLFW_STICKY_KEYS, GL_TRUE);
    // [BUG][glfw] glfwGetCursorPos does not update if cursor visible ???!!!
    // glfwSetInputMode(data_->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // VSync
    data_->vsync = props.vsync;
    glfwSwapInterval(props.vsync ? 1 : 0);

    // Send GLFW a raw pointer to our data structure so we can act on it from callbacks
    glfwSetWindowUserPointer(data_->window, &(*data_));

    set_event_callbacks(props);
}

void GLFWWindow::set_event_callbacks(const WindowProps& props)
{
    // Error callback
    glfwSetErrorCallback(GLFW_error_callback);

    // Window close event
    glfwSetWindowCloseCallback(data_->window, [](auto window) {
        auto* pw = static_cast<GLFWWindowDataImpl*>(glfwGetWindowUserPointer(window));
        pw->on_close();
    });

    // If we render to a GUI window for example, we rather react to the GUI window resize events.
    // So the following callbacks are not set if window is not host.
    if(props.host)
    {
        // Window resize event
        glfwSetWindowSizeCallback(data_->window, [](GLFWwindow* window, int width, int height) {
            auto* pw = static_cast<GLFWWindowDataImpl*>(glfwGetWindowUserPointer(window));
            pw->on_window_resize(width, height);
        });

        // On window resize, framebuffer needs resizing and glViewport must be called with the new size
        glfwSetFramebufferSizeCallback(data_->window, [](GLFWwindow* window, int width, int height) {
            auto* pw = static_cast<GLFWWindowDataImpl*>(glfwGetWindowUserPointer(window));
            pw->on_framebuffer_resize(width, height);
        });
    }

    // Keyboard event
    glfwSetKeyCallback(data_->window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto* pw = static_cast<GLFWWindowDataImpl*>(glfwGetWindowUserPointer(window));
        pw->on_keyboard(key, scancode, action, mods);
    });

    // Key typed event
    glfwSetCharCallback(data_->window, [](GLFWwindow* window, unsigned int codepoint) {
        auto* pw = static_cast<GLFWWindowDataImpl*>(glfwGetWindowUserPointer(window));
        pw->on_key_typed(codepoint);
    });

    // Mouse event
    glfwSetMouseButtonCallback(data_->window, [](GLFWwindow* window, int button, int action, int mods) {
        auto* pw = static_cast<GLFWWindowDataImpl*>(glfwGetWindowUserPointer(window));
        pw->on_mouse_button(button, action, mods);
    });

    // Cursor moving event
    glfwSetCursorPosCallback(data_->window, [](GLFWwindow* window, double x, double y) {
        auto* pw = static_cast<GLFWWindowDataImpl*>(glfwGetWindowUserPointer(window));
        pw->on_mouse_move(x, y);
    });

    // Mouse scroll event
    glfwSetScrollCallback(data_->window, [](GLFWwindow* window, double x_offset, double y_offset) {
        auto* pw = static_cast<GLFWWindowDataImpl*>(glfwGetWindowUserPointer(window));
        pw->on_mouse_scroll(x_offset, y_offset);
    });
}

void GLFWWindow::cleanup()
{
    W_PROFILE_FUNCTION()

    glfwDestroyWindow(data_->window);
    if(--s_glfw_num_windows == 0)
    {
        KLOGN("core") << "Shutting down GLFW." << std::endl;
        glfwTerminate();
    }
}

void GLFWWindow::update()
{
    W_PROFILE_FUNCTION()

    context_->swap_buffers();
    glfwPollEvents();
}

uint32_t GLFWWindow::get_width() const
{
    /*int width, height;
    glfwGetWindowSize(data_->window, &width, &height);
    return width;*/
    return uint32_t(data_->width);
}

uint32_t GLFWWindow::get_height() const
{
    /*int width, height;
    glfwGetWindowSize(data_->window, &width, &height);
    return height;*/
    return uint32_t(data_->height);
}

void GLFWWindow::set_vsync(bool value)
{
    data_->vsync = value;
    glfwSwapInterval(value ? 1 : 0);
}

bool GLFWWindow::is_vsync() { return data_->vsync; }

void* GLFWWindow::get_native() const { return static_cast<void*>(data_->window); }

} // namespace erwin