// clang-format off
#include "Platform/OGL/ogl_swapchain.h"
#include "../../window.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include <kibble/logger/logger.h>
// clang-format on

namespace gfx
{

OGLSwapchain::OGLSwapchain(const Window& window) : window_(window) {}

void OGLSwapchain::present() { glfwSwapBuffers(static_cast<GLFWwindow*>(window_.get_native())); }

} // namespace gfx