#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "ogl_context.h"
#include "../Erwin/debug/logger.h"
#include "../Erwin/core/core.h"

namespace erwin
{

OGLContext::OGLContext(void* window_handle):
window_handle_(window_handle)
{
	init();
}

OGLContext::~OGLContext()
{

}

void OGLContext::init()
{
	GLFWwindow* window = static_cast<GLFWwindow*>(window_handle_);
	glfwMakeContextCurrent(window);
	int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	if(!status)
	{
		DLOGF("core") << "Unable to load GLAD!" << std::endl;
		fatal();
	}

	DLOG("core", 1) << "[OpenGL]" << std::endl;
	DLOGI << "Vendor:   " << glGetString(GL_VENDOR) << std::endl;
	DLOGI << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
	DLOGI << "Version:  " << glGetString(GL_VERSION) << std::endl;

	// Pop any GL error that would have been caused by say... GLFW init
	glGetError();
}

void OGLContext::swap_buffers()
{
	glfwSwapBuffers(static_cast<GLFWwindow*>(window_handle_));
}


} // namespace erwin