#include "Platform/Vulkan/vulkan_engine_factory.h"
#include "Platform/GLFW/window.h"
#include "Platform/Vulkan/render_device.h"
#include "Platform/Vulkan/swap_chain.h"
// #include "Platform/Vulkan/device_context.h"

namespace gfx
{

template <>
EngineFactory::EngineComponents EngineFactory::create<DeviceAPI::Vulkan>(const EngineCreateInfo& create_info)
{
    auto p_window = std::make_unique<GLFWWindow>(create_info.window_props);
    auto p_render_device = std::make_unique<VKRenderDevice>();
    auto p_swap_chain = std::make_unique<VKSwapChain>(*p_render_device);

    p_render_device->create_instance(create_info.window_props.title);
    p_render_device->setup_debug_messenger();
    p_swap_chain->create_surface(*p_window);
    p_render_device->select_physical_device(p_swap_chain->get_surface());
    p_render_device->create_logical_device(p_swap_chain->get_surface());
    p_swap_chain->create(p_window->get_width(), p_window->get_height());

    return {std::move(p_window), std::move(p_render_device), std::move(p_swap_chain), nullptr};
}

} // namespace gfx