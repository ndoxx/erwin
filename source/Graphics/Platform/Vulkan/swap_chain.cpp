#include "Platform/Vulkan/swap_chain.h"
#include "../../window.h"
#include "GLFW/glfw3.h"
#include "Platform/GLFW/window.h"
#include "Platform/Vulkan/render_device.h"
#include "utils.h"
#include <kibble/logger/logger.h>

namespace gfx
{

VKSwapChain::VKSwapChain(const RenderDevice& rd) : render_device_(rd) {}

VKSwapChain::~VKSwapChain()
{
    KLOGN("render") << "[VK] Destroying swap chain." << std::endl;

    const auto& render_device = static_cast<const VKRenderDevice&>(render_device_);
    for(auto image_view : swap_chain_image_views_)
        render_device.get_logical_device().destroyImageView(image_view);

    render_device.get_logical_device().destroySwapchainKHR(swap_chain_);
    render_device.get_instance().destroySurfaceKHR(surface_);
}

void VKSwapChain::present() {}

void VKSwapChain::create_surface(const Window& window)
{
    const auto& render_device = static_cast<const VKRenderDevice&>(render_device_);
    auto* pw = static_cast<GLFWwindow*>(window.get_native());
    if(glfwCreateWindowSurface(render_device.get_instance(), pw, nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface_)) !=
       VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface!");
}

void VKSwapChain::create_swap_chain(const Window& window)
{
    KLOGN("render") << "[VK] Creating swap chain." << std::endl;
    const auto& render_device = static_cast<const VKRenderDevice&>(render_device_);

    auto swap_chain_support = query_swap_chain_support(render_device.get_physical_device(), surface_);
    auto surface_format = choose_swap_surface_format(swap_chain_support.formats);
    auto present_mode = choose_swap_present_mode(swap_chain_support.present_modes);
    auto extent = choose_swap_extent(window.get_width(), window.get_height(), swap_chain_support.capabilities);

    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
    if(swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
        image_count = swap_chain_support.capabilities.maxImageCount;

    vk::SwapchainCreateInfoKHR create_info(vk::SwapchainCreateFlagsKHR(), surface_, image_count, surface_format.format,
                                           surface_format.colorSpace, extent,
                                           1, // imageArrayLayers
                                           vk::ImageUsageFlagBits::eColorAttachment);

    // Typically, the indices for the graphics family and present family queues are the same, but we
    // can't be sure
    auto indices = find_queue_families(render_device.get_physical_device(), surface_);
    uint32_t queue_family_indices[] = {indices.graphics_family.value(), indices.present_family.value()};
    if(indices.graphics_family != indices.present_family)
    {
        create_info.imageSharingMode = vk::SharingMode::eConcurrent;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
        create_info.imageSharingMode = vk::SharingMode::eExclusive;
        create_info.queueFamilyIndexCount = 0;     // Optional
        create_info.pQueueFamilyIndices = nullptr; // Optional
    }

    // We don't need to support any image transform in the swap chain
    create_info.preTransform = swap_chain_support.capabilities.currentTransform;

    create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = vk::SwapchainKHR(nullptr);

    try
    {
        swap_chain_ = render_device.get_logical_device().createSwapchainKHR(create_info);
    }
    catch(vk::SystemError err)
    {
        throw std::runtime_error("Failed to create swap chain!");
    }

    // Retrieve swap chain image handles
    swap_chain_images_ = render_device.get_logical_device().getSwapchainImagesKHR(swap_chain_);
    swap_chain_image_format_ = surface_format.format;
    swap_chain_extent_ = extent;
}

void VKSwapChain::create_swap_chain_image_views()
{
    KLOGN("render") << "[VK] Creating swap chain image views." << std::endl;
    const auto& render_device = static_cast<const VKRenderDevice&>(render_device_);

    swap_chain_image_views_.resize(swap_chain_images_.size());
    for(size_t ii = 0; ii < swap_chain_images_.size(); ++ii)
    {
        vk::ImageViewCreateInfo create_info = {};
        create_info.image = swap_chain_images_[ii];
        create_info.viewType = vk::ImageViewType::e2D;
        create_info.format = swap_chain_image_format_;
        create_info.components.r = vk::ComponentSwizzle::eIdentity;
        create_info.components.g = vk::ComponentSwizzle::eIdentity;
        create_info.components.b = vk::ComponentSwizzle::eIdentity;
        create_info.components.a = vk::ComponentSwizzle::eIdentity;
        create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        try
        {
            swap_chain_image_views_[ii] = render_device.get_logical_device().createImageView(create_info);
        }
        catch(vk::SystemError err)
        {
            throw std::runtime_error("Failed to create image views!");
        }
    }
}

} // namespace gfx