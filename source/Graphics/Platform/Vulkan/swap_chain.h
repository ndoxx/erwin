#pragma once

#include "../../swap_chain.h"
#include <vector>
#include <vulkan/vulkan.h>

namespace gfx
{

class VKSwapChain : public SwapChain
{
public:
    VKSwapChain(const Window& window, const RenderDevice& rd);
    ~VKSwapChain();

    void present() override;

private:
    void create_swap_chain(const Window& window, const RenderDevice& rd);
    void create_swap_chain_image_views(const RenderDevice& rd);
    VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
    VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes);
    VkExtent2D choose_swap_extent(const Window& window, const VkSurfaceCapabilitiesKHR& capabilities);

private:
    VkSwapchainKHR swap_chain_;
    VkFormat swap_chain_image_format_;
    VkExtent2D swap_chain_extent_;
    std::vector<VkImage> swap_chain_images_;
    std::vector<VkImageView> swap_chain_image_views_;
    std::vector<VkFramebuffer> swap_chain_framebuffers_;
    const RenderDevice& render_device_;
};

} // namespace gfx