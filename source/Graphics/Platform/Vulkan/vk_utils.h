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

// Get a list of extensions required by GLFW, plus additionally the debug extension
std::vector<const char*> get_required_extensions();
// Check that our instance supports a list of required extensions
bool check_required_extensions(const std::vector<const char*>& required);
// Check support for a list of validation layers
bool check_validation_layer_support(const std::vector<const char*>& required);
// Create a debug messenger creation structure and hook it to the Kibble logger
vk::DebugUtilsMessengerCreateInfoEXT make_debug_messenger_create_info();
// Rate a physical device given its properties and whether it supports swapchain and various extensions
int rate_device_suitability(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface,
                            const std::vector<const char*>& extensions);
// Find indices of queue families in a physical device
QueueFamilyIndices find_queue_families(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface);
// Get a structure representing swapchain support for a physical device
SwapChainSupportDetails query_swapchain_support(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface);

} // namespace gfx