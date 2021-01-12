#pragma once

#include "../../swap_chain.h"
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
class VKSwapChain : public SwapChain
{
public:
    friend class EngineFactory;

    VKSwapChain(const VKRenderDevice& rd);
    ~VKSwapChain();

    void present() override;
    inline const vk::SurfaceKHR& get_surface() const { return surface_; }

private:
    // Create a window surface from a Window instance
    void create_surface(const Window& window);
    // Create or re-create the swap chain and all its components
    void create(uint32_t width, uint32_t height);

    // Helper methods called sequentially during create()
    void create_swap_chain(uint32_t width, uint32_t height);
    void create_swap_chain_image_views();

private:
    vk::SwapchainKHR swap_chain_;
    vk::SurfaceFormatKHR swap_chain_format_;
    vk::Extent2D swap_chain_extent_;
    vk::SurfaceKHR surface_;
    std::vector<SwapchainImage> swap_chain_images_;
    std::vector<vk::Framebuffer> swap_chain_framebuffers_;
    const VKRenderDevice& render_device_;
};

} // namespace gfx