#pragma once
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

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
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

std::vector<const char*> get_required_extensions();
bool check_required_extensions(const std::vector<const char*>& required);
bool check_validation_layer_support(const std::vector<const char*>& required);
void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info);
int rate_device_suitability(const VkPhysicalDevice& device, const VkSurfaceKHR& surface,
                            const std::vector<const char*>& extensions);
bool check_device_extensions_support(const VkPhysicalDevice& device, const std::vector<const char*>& extensions);
QueueFamilyIndices find_queue_families(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
SwapChainSupportDetails query_swap_chain_support(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats);
VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes);
VkExtent2D choose_swap_extent(uint32_t width, uint32_t height, const VkSurfaceCapabilitiesKHR& capabilities);

} // namespace gfx