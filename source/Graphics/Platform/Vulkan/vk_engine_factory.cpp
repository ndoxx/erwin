#include "Platform/Vulkan/vk_engine_factory.h"
#include "Platform/GLFW/glfw_window.h"
#include "Platform/Vulkan/vk_render_device.h"
#include "Platform/Vulkan/vk_swapchain.h"
#include "Platform/Vulkan/vk_render_context.h"
#include <kibble/logger/logger.h>

namespace gfx
{

template <>
EngineFactory::EngineComponents EngineFactory::create<DeviceAPI::Vulkan>(const EngineCreateInfo& create_info)
{
	log_create_info(create_info);

    auto p_window = std::make_unique<GLFWWindow>(create_info.window_props);
    auto p_render_device = std::make_unique<VKRenderDevice>();
    auto p_swapchain = std::make_unique<VKSwapchain>(*p_render_device);
    auto p_context = std::make_unique<VKRenderContext>(*p_render_device, *p_swapchain);

    p_render_device->create_instance(create_info);
    p_render_device->setup_debug_messenger();
    p_swapchain->create_surface(*p_window);
    p_render_device->select_physical_device(p_swapchain->get_surface());
    p_render_device->create_logical_device(p_swapchain->get_surface());
    p_swapchain->create(p_window->get_width(), p_window->get_height());

    p_context->init(create_info.renderer_config.max_sample_count);

    return {std::move(p_window), std::move(p_render_device), std::move(p_swapchain), std::move(p_context)};
}

} // namespace gfx