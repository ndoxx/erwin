#include "window.h"
#include "Platform/GLFW/window.h"
#include <kibble/logger/logger.h>

namespace gfx
{
std::unique_ptr<Window> Window::create(DeviceAPI api, const WindowProps& props)
{
    switch(api)
    {
    case DeviceAPI::OpenGL:
        return std::make_unique<GLFWWindow>(api, props);
    default: {
        return std::make_unique<GLFWWindow>(api, props);
    }
    }
}

} // namespace gfx
