#include "Platform/OGL/ogl_engine_factory.h"
#include "Platform/GLFW/window.h"
#include "Platform/OGL/render_device.h"
#include "Platform/OGL/swap_chain.h"
// #include "Platform/OGL/device_context.h"

namespace gfx
{

template <>
EngineFactory::EngineComponents EngineFactory::create<DeviceAPI::OpenGL>(const EngineCreateInfo& create_info)
{
	auto p_window = std::make_unique<GLFWWindow>(create_info.window_props);
	auto p_render_device = std::make_unique<OGLRenderDevice>(*p_window);
	auto p_swap_chain = std::make_unique<OGLSwapChain>(*p_window);

    return {std::move(p_window), std::move(p_render_device), std::move(p_swap_chain), nullptr};
}

}