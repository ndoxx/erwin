#include "Platform/Vulkan/swap_chain.h"
#include "../../window.h"
#include "GLFW/glfw3.h"
#include "Platform/GLFW/window.h"
#include "Platform/Vulkan/render_device.h"
#include <kibble/logger/logger.h>

namespace gfx
{

VKSwapChain::VKSwapChain(const RenderDevice& rd) : render_device_(rd) {}

VKSwapChain::~VKSwapChain()
{
    const auto& render_device = static_cast<const VKRenderDevice&>(render_device_);
    for(auto&& view : swap_chain_image_views_)
        vkDestroyImageView(render_device.get_logical_device(), view, nullptr);

    vkDestroySwapchainKHR(render_device.get_logical_device(), swap_chain_, nullptr);
    vkDestroySurfaceKHR(render_device.get_instance(), surface_, nullptr);
}

void VKSwapChain::present() {}

void VKSwapChain::create_surface(const Window& window)
{
    const auto& render_device = static_cast<const VKRenderDevice&>(render_device_);
    auto* pw = static_cast<GLFWwindow*>(window.get_native());
    if(glfwCreateWindowSurface(render_device.get_instance(), pw, nullptr, &surface_) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface!");
}

void VKSwapChain::create_swap_chain(const Window& window)
{
    KLOGN("render") << "[VK] Creating swap chain." << std::endl;
    const auto& render_device = static_cast<const VKRenderDevice&>(render_device_);

    auto swap_chain_support = query_swap_chain_support(render_device.get_physical_device(), surface_);
    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swap_chain_support.formats);
    VkPresentModeKHR present_mode = choose_swap_present_mode(swap_chain_support.present_modes);
    VkExtent2D extent = choose_swap_extent(window.get_width(), window.get_height(), swap_chain_support.capabilities);

    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1u;
    if(swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount)
        image_count = swap_chain_support.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface_;
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Typically, the indices for the graphics family and present family queues are the same, but we
    // can't be sure
    auto indices = find_queue_families(render_device.get_physical_device(), surface_);
    uint32_t queue_family_indices[] = {indices.graphics_family.value(), indices.present_family.value()};
    if(indices.graphics_family != indices.present_family)
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;     // Optional
        create_info.pQueueFamilyIndices = nullptr; // Optional
    }

    // We don't need to support any image transform in the swap chain
    create_info.preTransform = swap_chain_support.capabilities.currentTransform;

    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    if(vkCreateSwapchainKHR(render_device.get_logical_device(), &create_info, nullptr, &swap_chain_) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swap chain!");

    // Retrieve swap chain image handles
    vkGetSwapchainImagesKHR(render_device.get_logical_device(), swap_chain_, &image_count, nullptr);
    swap_chain_images_.resize(image_count);
    vkGetSwapchainImagesKHR(render_device.get_logical_device(), swap_chain_, &image_count, swap_chain_images_.data());

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
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swap_chain_images_[ii];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swap_chain_image_format_;

        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if(vkCreateImageView(render_device.get_logical_device(), &create_info, nullptr, &swap_chain_image_views_[ii]) !=
           VK_SUCCESS)
            throw std::runtime_error("Failed to create image views!");
    }
}

} // namespace gfx