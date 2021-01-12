// clang-format off
#include "Platform/GLFW/glfw_window.h"
#include "Platform/OGL/ogl_engine_factory.h"
#include "Platform/OGL/ogl_render_device.h"
#include "Platform/OGL/ogl_swapchain.h"
// #include "Platform/OGL/ogl_device_context.h"
// clang-format on

namespace gfx
{

template <>
EngineFactory::EngineComponents EngineFactory::create<DeviceAPI::OpenGL>(const EngineCreateInfo& create_info)
{
    auto p_window = std::make_unique<GLFWWindow>(create_info.window_props);
    auto p_render_device = std::make_unique<OGLRenderDevice>(*p_window);
    auto p_swapchain = std::make_unique<OGLSwapchain>(*p_window);

    return {std::move(p_window), std::move(p_render_device), std::move(p_swapchain), nullptr};
}

} // namespace gfx