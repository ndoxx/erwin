#include "Platform/OGL/swap_chain.h"
#include "../../window.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <kibble/logger/logger.h>

namespace gfx
{

OGLSwapChain::OGLSwapChain(const Window& window):
window_(window)
{

}

void OGLSwapChain::present()
{
	glfwSwapBuffers(static_cast<GLFWwindow*>(window_.get_native()));
}

} // namespace gfx