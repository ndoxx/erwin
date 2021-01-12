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
    friend class EngineFactory;

    VKSwapchain(const VKRenderDevice& rd);
    ~VKSwapchain();

    void present() override;
    inline const vk::SurfaceKHR& get_surface() const { return surface_; }

private:
    // Create a window surface from a Window instance
    void create_surface(const Window& window);
    // Create or re-create the swap chain and all its components
    void create(uint32_t width, uint32_t height);

    // Helper methods called sequentially during create()
    void create_swapchain(uint32_t width, uint32_t height);
    void create_swapchain_image_views();

private:
    vk::SwapchainKHR swapchain_;
    vk::SurfaceFormatKHR swapchain_format_;
    vk::Extent2D swapchain_extent_;
    vk::SurfaceKHR surface_;
    std::vector<SwapchainImage> swapchain_images_;
    const VKRenderDevice& render_device_;
};

} // namespace gfx