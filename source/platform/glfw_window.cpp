#include "GLFW/glfw3.h"

#include "glfw_window.h"
#include "glfw_keymap.h"
#include "ogl_context.h"

#include "../Erwin/debug/logger.h"
#include "../Erwin/core/core.h"

#include "../Erwin/event/window_events.h"

namespace erwin
{

static bool glfw_initialized = false;

Window* Window::create(const WindowProps& props)
{
	return new GLFWWindow(props);
}

static void GLFW_error_callback(int error, const char* description)
{
	DLOGE("core") << "GLFW Error (" << error << "): " << description << std::endl;
}

struct GLFWWindow::GLFWWindowDataImpl
{
	GLFWwindow* window;
	bool vsync;
	int width;
	int height;
	std::string title;
};

GLFWWindow::GLFWWindow(const WindowProps& props):
data_(std::make_unique<GLFWWindowDataImpl>())
{
	init(props);
}

GLFWWindow::~GLFWWindow()
{
	cleanup();
}

void GLFWWindow::init(const WindowProps& props)
{
	// Initialize GLFW if not already initialized
	if(!glfw_initialized)
	{
		if(!glfwInit())
		{
			DLOGF("core") << "Failed to initialize GLFW." << std::endl;
			fatal();
		}

		DLOG("core",0) << "[GLFW]" << std::endl;
		// Show GLFW version
	    {
	        int major, minor, rev;
	        glfwGetVersion(&major, &minor, &rev);
	        DLOGI << "Version:  " << major << "." << minor << "." << rev << std::endl;
	    }

		glfw_initialized = true;
	}

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window
    if(props.full_screen)
        data_->window = glfwCreateWindow(props.width, props.height, props.title.c_str(), glfwGetPrimaryMonitor(), NULL);
    else
        data_->window = glfwCreateWindow(props.width, props.height, props.title.c_str(), NULL, NULL);

    data_->width = props.width;
    data_->height = props.height;
    data_->title = props.title;

    if(data_->window == NULL)
    {
        DLOGF("core") << "Failed to open GLFW window." << std::endl;
        fatal();
    }

    // Make window always-on-top if required
    if(!props.full_screen && props.always_on_top)
        glfwSetWindowAttrib(data_->window, GLFW_FLOATING, GLFW_TRUE);

    context_ = new OGLContext(data_->window);

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(data_->window, GLFW_STICKY_KEYS, GL_TRUE);
    // [BUG][glfw] glfwGetCursorPos does not update if cursor visible ???!!!
    //glfwSetInputMode(data_->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //VSync
    data_->vsync = props.vsync;
    if(data_->vsync)
        glfwSwapInterval(1); // Enable vsync

    // Send GLFW a raw pointer to our data structure so we can act on it from callbacks
    glfwSetWindowUserPointer(data_->window, &(*data_));

    set_event_callbacks();
}

void GLFWWindow::set_event_callbacks()
{
	// Error callback
	glfwSetErrorCallback(GLFW_error_callback);

	// Window close event
	glfwSetWindowCloseCallback(data_->window, [](GLFWwindow* window)
	{
		EVENTBUS.publish(WindowCloseEvent());
	});

	// Window resize event
	glfwSetWindowSizeCallback(data_->window, [](GLFWwindow* window, int width, int height)
	{
		auto* data = static_cast<GLFWWindowDataImpl*>(glfwGetWindowUserPointer(window));
		data->width = width;
		data->height = height;
		EVENTBUS.publish(WindowResizeEvent(width, height));
	});

	// Keyboard event
	glfwSetKeyCallback(data_->window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		keymap::WKEY wkey = keymap::GLFW_KEY_TO_WKEY.at(key);
		switch(action)
		{
			case GLFW_PRESS:
			{
				EVENTBUS.publish(KeyboardEvent(wkey, mods, true, false));
				break;
			}
			case GLFW_RELEASE:
			{
				EVENTBUS.publish(KeyboardEvent(wkey, mods, false, false));
				break;
			}
			case GLFW_REPEAT:
			{
				EVENTBUS.publish(KeyboardEvent(wkey, mods, true, true));
				break;
			}
		}
	});

	// Key typed event
	glfwSetCharCallback(data_->window, [](GLFWwindow* window, unsigned int codepoint)
	{
		EVENTBUS.publish(KeyTypedEvent(codepoint));
	});

	// Mouse event
	glfwSetMouseButtonCallback(data_->window, [](GLFWwindow* window, int button, int action, int mods)
	{
		double x,y;
    	glfwGetCursorPos(window, &x, &y);

    	keymap::WMOUSE wbutton = keymap::GLFW_MB_TO_WMOUSE.at(button);

		switch(action)
		{
			case GLFW_PRESS:
			{
				EVENTBUS.publish(MouseButtonEvent(wbutton,mods,true,x,y));
				break;
			}
			case GLFW_RELEASE:
			{
				EVENTBUS.publish(MouseButtonEvent(wbutton,mods,false,x,y));
				break;
			}
		}
	});

	// Cursor moving event
	glfwSetCursorPosCallback(data_->window, [](GLFWwindow* window, double x, double y)
	{
		EVENTBUS.publish(MouseMovedEvent(x, y));
	});

	// Mouse scroll event
	glfwSetScrollCallback(data_->window, [](GLFWwindow* window, double x_offset, double y_offset)
	{
		EVENTBUS.publish(MouseScrollEvent(x_offset, y_offset));
	});
}

void GLFWWindow::cleanup()
{
	glfwDestroyWindow(data_->window);
	// Will need to remove this in case of multi-window application
    glfwTerminate();
}


void GLFWWindow::update()
{
	context_->swap_buffers();
	glfwPollEvents();
}

uint32_t GLFWWindow::get_width() const
{
	int width, height;
	glfwGetWindowSize(data_->window, &width, &height);
	return width;
}

uint32_t GLFWWindow::get_height() const
{
	int width, height;
	glfwGetWindowSize(data_->window, &width, &height);
	return height;
}

void GLFWWindow::set_vsync(bool value)
{
	data_->vsync = value;
	glfwSwapInterval(value ? 1 : 0);
}

bool GLFWWindow::is_vsync()
{
	return data_->vsync;
}

void* GLFWWindow::get_native() const
{
	return static_cast<void*>(data_->window);
}

} // namespace erwin