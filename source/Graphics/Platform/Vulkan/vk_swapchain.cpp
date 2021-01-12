// clang-format off
#include "../../window.h"
#include "Platform/GLFW/glfw_window.h"
#include "Platform/Vulkan/vk_swapchain.h"
#include "Platform/Vulkan/vk_render_device.h"
#include "Platform/Vulkan/vk_utils.h"
#include <kibble/logger/logger.h>
#include <stack>
#include <map>
#include "GLFW/glfw3.h"
// clang-format on

using namespace kb;

namespace gfx
{

// clang-format off
const std::map<vk::PresentModeKHR, std::string> s_str_present_modes = 
{
    {vk::PresentModeKHR::eImmediate,   "Immediate"},
    {vk::PresentModeKHR::eFifoRelaxed, "FifoRelaxed"},
    {vk::PresentModeKHR::eFifo,        "Fifo"},
    {vk::PresentModeKHR::eMailbox,     "Mailbox"},
};
// clang-format on

VKSwapchain::VKSwapchain(const VKRenderDevice& rd) : render_device_(rd) {}

VKSwapchain::~VKSwapchain()
{
    KLOGN("render") << "[VK] Destroying swap chain." << std::endl;

    for(auto& image : swapchain_images_)
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

    swapchain_images_.clear();
    render_device_.get_logical_device().destroySwapchainKHR(swapchain_);
    render_device_.get_instance().destroySurfaceKHR(surface_);
}

void VKSwapchain::present() {}

void VKSwapchain::create_surface(const Window& window)
{
    KLOGN("render") << "[VK] Creating window surface." << std::endl;
    auto* pw = static_cast<GLFWwindow*>(window.get_native());
    if(glfwCreateWindowSurface(render_device_.get_instance(), pw, nullptr,
                               reinterpret_cast<VkSurfaceKHR*>(&surface_)) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface!");
}

void VKSwapchain::create(uint32_t width, uint32_t height)
{
    KLOGN("render") << "[VK] Creating swapchain." << std::endl;
    build_swapchain(width, height);
    build_swapchain_image_views();
}

void VKSwapchain::build_swapchain(uint32_t width, uint32_t height)
{
    KLOG("render", 1) << "[VK] Creating swapchain instance." << std::endl;

    vk::SwapchainKHR old_swapchain = swapchain_;

    auto swapchain_support = query_swapchain_support(render_device_.get_physical_device(), surface_);
    auto surface_format = choose_swap_surface_format(swapchain_support.formats);
    auto present_mode = choose_swap_present_mode(swapchain_support.present_modes);
    auto extent = choose_swap_extent(width, height, swapchain_support.capabilities);

    // Log
    KLOGI << "Present mode: " << KS_VALU_ << s_str_present_modes.at(present_mode) << std::endl;
    KLOGI << "Extent:       " << KS_VALU_ << extent.width << 'x' << extent.height << std::endl;

    uint32_t desired_image_count = swapchain_support.capabilities.minImageCount + 1;
    if(swapchain_support.capabilities.maxImageCount > 0 &&
       desired_image_count > swapchain_support.capabilities.maxImageCount)
        desired_image_count = swapchain_support.capabilities.maxImageCount;

    vk::SwapchainCreateInfoKHR create_info(vk::SwapchainCreateFlagsKHR(), surface_, desired_image_count,
                                           surface_format.format, surface_format.colorSpace, extent,
                                           1, // imageArrayLayers
                                           vk::ImageUsageFlagBits::eColorAttachment);

    // Typically, the indices for the graphics family and present family queues are the same, but we
    // can't be sure
    auto indices = find_queue_families(render_device_.get_physical_device(), surface_);
    uint32_t queue_family_indices[] = {indices.graphics_family.value(), indices.present_family.value()};
    KLOGI << "PQueue:       " << KS_VALU_;
    if(indices.graphics_family != indices.present_family)
    {
        // If the device has a discrete presentation queue, images are allowed to be shared
        // between the graphics and presentation queues.
        create_info.imageSharingMode = vk::SharingMode::eConcurrent;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
        KLOGI << "discrete" << std::endl;
    }
    else
    {
        create_info.imageSharingMode = vk::SharingMode::eExclusive;
        create_info.queueFamilyIndexCount = 0;     // Optional
        create_info.pQueueFamilyIndices = nullptr; // Optional
        KLOGI << "non-discrete" << std::endl;
    }

    // Support for image transform in the swap chain
    if(swapchain_support.capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
        create_info.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    else
        create_info.preTransform = swapchain_support.capabilities.currentTransform;

    create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = old_swapchain;
    create_info.imageColorSpace = surface_format.colorSpace;

    try
    {
        swapchain_ = render_device_.get_logical_device().createSwapchainKHR(create_info);
    }
    catch(vk::SystemError err)
    {
        throw std::runtime_error("Failed to create swapchain!");
    }

    // If an existing swap chain is re-created, destroy the old swap chain and image views.
    if(old_swapchain)
    {
        KLOG("render", 0) << "[VK] Destroying old swapchain." << std::endl;
        for(size_t ii = 0; ii < swapchain_images_.size(); ++ii)
            render_device_.get_logical_device().destroyImageView(swapchain_images_[ii].view);
        render_device_.get_logical_device().destroySwapchainKHR(old_swapchain);
    }

    swapchain_format_ = surface_format;
    swapchain_extent_ = extent;
}

void VKSwapchain::build_swapchain_image_views()
{
    KLOG("render", 1) << "[VK] Creating swapchain image views." << std::endl;
    // Retrieve swap chain image handles
    auto swapchain_images = render_device_.get_logical_device().getSwapchainImagesKHR(swapchain_);
    swapchain_images_.resize(swapchain_images.size());
    KLOGI << "Image count:  " << KS_VALU_ << swapchain_images_.size() << std::endl;

    // Create image views for each swap chain image
    // clang-format off
    for(size_t ii = 0; ii < swapchain_images_.size(); ++ii)
    {
        swapchain_images_[ii].image = swapchain_images[ii];

        try
        {
            swapchain_images_[ii].view = render_device_.create_image_view(swapchain_images_[ii].image, 
                                                                          swapchain_format_.format, 
                                                                          vk::ImageAspectFlagBits::eColor, 
                                                                          1 /* mip levels */);
        }
        catch(vk::SystemError err)
        {
            throw std::runtime_error("Failed to create image views!");
        }
    }
    // clang-format on
}

vk::SurfaceFormatKHR VKSwapchain::choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats)
{
    if(available_formats.size() == 1 && available_formats[0].format == vk::Format::eUndefined)
        return {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};

    for(const auto& available_format : available_formats)
    {
        if(available_format.format == vk::Format::eB8G8R8A8Unorm &&
           available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            return available_format;
    }

    return available_formats[0];
}

vk::PresentModeKHR VKSwapchain::choose_swap_present_mode(const std::vector<vk::PresentModeKHR>& available)
{
    // Present modes are stacked, the favorite one (mailbox) is on top
    std::stack<vk::PresentModeKHR> preferences;
    preferences.push(vk::PresentModeKHR::eImmediate);
    preferences.push(vk::PresentModeKHR::eFifoRelaxed);
    preferences.push(vk::PresentModeKHR::eFifo);
    preferences.push(vk::PresentModeKHR::eMailbox);

    // Pop the stack till we find a present mode that is available
    while(!preferences.empty())
    {
        vk::PresentModeKHR mode{preferences.top()};
        if(std::find(available.begin(), available.end(), mode) != available.end())
            return mode;

        preferences.pop();
    }

    throw std::runtime_error("No compatible presentation mode found.");
}

vk::Extent2D VKSwapchain::choose_swap_extent(uint32_t width, uint32_t height,
                                             const vk::SurfaceCapabilitiesKHR& capabilities)
{
    if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;
    else
    {
        vk::Extent2D actual_extent = {width, height};
        std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actual_extent;
    }
}

} // namespace gfx