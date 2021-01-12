#pragma once

/*
 * The Vulkan swap chain abstracts an array of presentable images associated
 * to the window surface.
 * The VKSwapchain class wraps a Vulkan swap chain together with the surface
 * and images, plus additional extent and format properties.
 */

#include "../../swapchain.h"
#include <vector>
#include <vulkan/vulkan.hpp>

namespace gfx
{

struct SwapchainImage
{
    vk::Image image;
    vk::ImageView view;
    vk::Fence fence;
};

class Window;
class VKRenderDevice;
class VKSwapchain : public Swapchain
{
public:
    VKSwapchain(const VKRenderDevice& rd);
    ~VKSwapchain();

    void present() override;
    inline const vk::SurfaceKHR& get_surface() const { return surface_; }

    // Create a window surface from a Window instance
    void create_surface(const Window& window);
    // Create or re-create the swap chain and all its components
    void create(uint32_t width, uint32_t height);

private:
    // Helper methods called sequentially during create()
    void build_swapchain(uint32_t width, uint32_t height);
    void build_swapchain_image_views();

    // Helper methods for swapchain construction
    // Acquire suitable swapchain format
    vk::SurfaceFormatKHR choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats);
    // Choose the best present mode available
    vk::PresentModeKHR choose_swap_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes);
    // Constrain the swapchain extent
    vk::Extent2D choose_swap_extent(uint32_t width, uint32_t height, const vk::SurfaceCapabilitiesKHR& capabilities);

private:
    vk::SurfaceKHR surface_;
    vk::SwapchainKHR swapchain_;
    vk::SurfaceFormatKHR swapchain_format_;
    vk::Extent2D swapchain_extent_;
    std::vector<SwapchainImage> swapchain_images_;
    const VKRenderDevice& render_device_;
};

} // namespace gfx