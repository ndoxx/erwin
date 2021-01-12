#pragma once

#include "../../swap_chain.h"
#include <vector>
#include <vulkan/vulkan.hpp>

namespace gfx
{

class VKSwapChain : public SwapChain
{
public:
    VKSwapChain(const RenderDevice& rd);
    ~VKSwapChain();

    void present() override;
    inline const vk::SurfaceKHR& get_surface() const { return surface_; }

    friend class EngineFactory;

private:
    void create_surface(const Window& window);
    void create_swap_chain(const Window& window);
    void create_swap_chain_image_views();

private:
    vk::SwapchainKHR swap_chain_;
    vk::Format swap_chain_image_format_;
    vk::Extent2D swap_chain_extent_;
    vk::SurfaceKHR surface_;
    std::vector<vk::Image> swap_chain_images_;
    std::vector<vk::ImageView> swap_chain_image_views_;
    std::vector<vk::Framebuffer> swap_chain_framebuffers_;
    const RenderDevice& render_device_;
};

} // namespace gfx