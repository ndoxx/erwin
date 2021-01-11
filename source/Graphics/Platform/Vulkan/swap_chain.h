#pragma once

#include "../../swap_chain.h"
#include <vector>
#include <vulkan/vulkan.h>

namespace gfx
{

class VKSwapChain : public SwapChain
{
public:
    VKSwapChain(const RenderDevice& rd);
    ~VKSwapChain();

    void present() override;
    inline const VkSurfaceKHR& get_surface() const { return surface_; }

    friend class EngineFactory;

private:
    void create_surface(const Window& window);
    void create_swap_chain(const Window& window);
    void create_swap_chain_image_views();

private:
    VkSwapchainKHR swap_chain_;
    VkFormat swap_chain_image_format_;
    VkExtent2D swap_chain_extent_;
    VkSurfaceKHR surface_;
    std::vector<VkImage> swap_chain_images_;
    std::vector<VkImageView> swap_chain_image_views_;
    std::vector<VkFramebuffer> swap_chain_framebuffers_;
    const RenderDevice& render_device_;
};

} // namespace gfx