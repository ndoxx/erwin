#include "Platform/Vulkan/swap_chain.h"
#include "../../window.h"
#include "GLFW/glfw3.h"
#include "Platform/GLFW/window.h"
#include "Platform/Vulkan/render_device.h"
#include "utils.h"
#include <kibble/logger/logger.h>

namespace gfx
{

VKSwapChain::VKSwapChain(const VKRenderDevice& rd) : render_device_(rd) {}

VKSwapChain::~VKSwapChain()
{
    KLOGN("render") << "[VK] Destroying swap chain." << std::endl;

    for(auto& image : swap_chain_images_)
    {
        if(image.fence)
        {
            auto result = render_device_.get_logical_device().waitForFences(image.fence, VK_TRUE, UINT64_MAX);
            if(result == vk::Result::eTimeout)
            {
                KLOGW("render") << "[VK] Swap chain image fence timeout." << std::endl;
            }
            else if(result != vk::Result::eSuccess)
            {
                KLOGE("render") << "[VK] Error encountered while waiting for swap chain image fence." << std::endl;
            }
            render_device_.get_logical_device().destroyFence(image.fence);
        }
        render_device_.get_logical_device().destroyImageView(image.view);
    }

    swap_chain_images_.clear();
    render_device_.get_logical_device().destroySwapchainKHR(swap_chain_);
    render_device_.get_instance().destroySurfaceKHR(surface_);
}

void VKSwapChain::present() {}

void VKSwapChain::create_surface(const Window& window)
{
    KLOGN("render") << "[VK] Creating window surface." << std::endl;
    auto* pw = static_cast<GLFWwindow*>(window.get_native());
    if(glfwCreateWindowSurface(render_device_.get_instance(), pw, nullptr,
                               reinterpret_cast<VkSurfaceKHR*>(&surface_)) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface!");
}

void VKSwapChain::create(uint32_t width, uint32_t height)
{
    KLOGN("render") << "[VK] Creating swap chain." << std::endl;
    create_swap_chain(width, height);
    create_swap_chain_image_views();
}

void VKSwapChain::create_swap_chain(uint32_t width, uint32_t height)
{
    KLOG("render", 1) << "[VK] Creating swap chain instance." << std::endl;

    vk::SwapchainKHR old_swap_chain = swap_chain_;

    auto swap_chain_support = query_swap_chain_support(render_device_.get_physical_device(), surface_);
    auto surface_format = choose_swap_surface_format(swap_chain_support.formats);
    auto present_mode = choose_swap_present_mode(swap_chain_support.present_modes);
    auto extent = choose_swap_extent(width, height, swap_chain_support.capabilities);

    uint32_t desired_image_count = swap_chain_support.capabilities.minImageCount + 1;
    if(swap_chain_support.capabilities.maxImageCount > 0 &&
       desired_image_count > swap_chain_support.capabilities.maxImageCount)
        desired_image_count = swap_chain_support.capabilities.maxImageCount;

    vk::SwapchainCreateInfoKHR create_info(vk::SwapchainCreateFlagsKHR(), surface_, desired_image_count,
                                           surface_format.format, surface_format.colorSpace, extent,
                                           1, // imageArrayLayers
                                           vk::ImageUsageFlagBits::eColorAttachment);

    // Typically, the indices for the graphics family and present family queues are the same, but we
    // can't be sure
    auto indices = find_queue_families(render_device_.get_physical_device(), surface_);
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

    // Support for image transform in the swap chain
    if(swap_chain_support.capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
        create_info.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    else
        create_info.preTransform = swap_chain_support.capabilities.currentTransform;

    create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = old_swap_chain;
    create_info.imageColorSpace = surface_format.colorSpace;

    try
    {
        swap_chain_ = render_device_.get_logical_device().createSwapchainKHR(create_info);
    }
    catch(vk::SystemError err)
    {
        throw std::runtime_error("Failed to create swap chain!");
    }

    // If an existing sawp chain is re-created, destroy the old swap chain
    // This also cleans up all the presentable images
    if(old_swap_chain)
    {
        for(size_t ii = 0; ii < swap_chain_images_.size(); ++ii)
            render_device_.get_logical_device().destroyImageView(swap_chain_images_[ii].view);
        render_device_.get_logical_device().destroySwapchainKHR(old_swap_chain);
    }

    swap_chain_format_ = surface_format;
    swap_chain_extent_ = extent;
}

void VKSwapChain::create_swap_chain_image_views()
{
    KLOG("render", 1) << "[VK] Creating swap chain image views." << std::endl;

    // Retrieve swap chain image handles
    auto swap_chain_images = render_device_.get_logical_device().getSwapchainImagesKHR(swap_chain_);

    // Create image views for each swap chain image
    vk::ImageViewCreateInfo view_create_info = {};
    view_create_info.viewType = vk::ImageViewType::e2D;
    view_create_info.format = swap_chain_format_.format;
    view_create_info.components.r = vk::ComponentSwizzle::eIdentity;
    view_create_info.components.g = vk::ComponentSwizzle::eIdentity;
    view_create_info.components.b = vk::ComponentSwizzle::eIdentity;
    view_create_info.components.a = vk::ComponentSwizzle::eIdentity;
    view_create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount = 1;

    swap_chain_images_.resize(swap_chain_images_.size());
    for(size_t ii = 0; ii < swap_chain_images_.size(); ++ii)
    {
        swap_chain_images_[ii].image = swap_chain_images[ii];
        view_create_info.image = swap_chain_images_[ii].image;

        try
        {
            swap_chain_images_[ii].view = render_device_.get_logical_device().createImageView(view_create_info);
        }
        catch(vk::SystemError err)
        {
            throw std::runtime_error("Failed to create image views!");
        }
    }
}

} // namespace gfx