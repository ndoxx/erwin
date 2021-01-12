#pragma once
#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>

#define GFX_VULKAN_ENABLE_VALIDATION_LAYERS

namespace gfx
{

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;

    inline bool is_complete() { return graphics_family.has_value() && present_family.has_value(); }
};

struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> present_modes;
};

std::vector<const char*> get_required_extensions();
bool check_required_extensions(const std::vector<const char*>& required);
bool check_validation_layer_support(const std::vector<const char*>& required);
vk::DebugUtilsMessengerCreateInfoEXT make_debug_messenger_create_info();
int rate_device_suitability(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface,
                            const std::vector<const char*>& extensions);
bool check_device_extensions_support(const vk::PhysicalDevice& device, const std::vector<const char*>& extensions);
QueueFamilyIndices find_queue_families(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface);
SwapChainSupportDetails query_swapchain_support(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface);
vk::SurfaceFormatKHR choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& available_formats);
vk::PresentModeKHR choose_swap_present_mode(const std::vector<vk::PresentModeKHR>& available_present_modes);
vk::Extent2D choose_swap_extent(uint32_t width, uint32_t height, const vk::SurfaceCapabilitiesKHR& capabilities);

} // namespace gfx