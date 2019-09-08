#include "glfw_window.h"
#include "GLFW/glfw3.h"
#include "../Erwin/debug/logger.h"
#include "../Erwin/core/core.h"

namespace erwin
{

static bool glfw_initialized = false;

Window* Window::create(const WindowProps& props)
{
	return new GLFWWindow(props);
}

struct GLFWWindow::GLFWWindowDataImpl
{
	GLFWwindow* window;
	bool vsync;
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

		glGetError(); // Hide glfwInit's errors
		glfw_initialized = true;
	}

	// Show GLFW version
    {
        int major, minor, rev;
        glfwGetVersion(&major, &minor, &rev);
        DLOG("core",0) << "GLFW version is: " << major << "." << minor << "." << rev << std::endl;
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

    if(data_->window == NULL)
    {
        DLOGF("core") << "Failed to open GLFW window." << std::endl;
        fatal();
    }

    // Make window always-on-top if required
    if(!props.full_screen && props.always_on_top)
        glfwSetWindowAttrib(data_->window, GLFW_FLOATING, GLFW_TRUE);

    glfwMakeContextCurrent(data_->window);
    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(data_->window, GLFW_STICKY_KEYS, GL_TRUE);
    // [BUG][glfw] glfwGetCursorPos does not update if cursor visible ???!!!
    glfwSetInputMode(data_->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //VSync
    data_->vsync = props.vsync;
    if(data_->vsync)
        glfwSwapInterval(1); // Enable vsync
}

void GLFWWindow::cleanup()
{
	glfwDestroyWindow(data_->window);
}


void GLFWWindow::update()
{

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